#include "GSLAM/core/GSLAM.h"

#include "evnsq/consumer.h"
#include "evnsq/producer.h"
#include "evpp/event_loop.h"

#ifdef __unix
#include <unistd.h>
#endif

using namespace GSLAM;

class SvarNSQ: public SvarValue// The wrapper is used to distinguish where is the message come from
{
public:
    SvarNSQ(GSLAM::Svar var):_var(var){}

    virtual TypeID          cpptype()const{return _var.cpptype();}
    virtual const void*     ptr() const{return _var.value()->ptr();}
    virtual const Svar&     classObject()const{return _var.value()->classObject();}
    virtual size_t          length() const {return _var.value()->length();}
    virtual std::mutex*     accessMutex()const{return _var.value()->accessMutex();}

    static Svar wrap(GSLAM::Svar var){
        return Svar((SvarValue*)new SvarNSQ(var));
    }

    GSLAM::Svar _var;
};

class SubscriberNSQ{
public:
    SubscriberNSQ(evpp::EventLoop& loop,Subscriber gslam_sub,
                  const std::string& nsqd_tcp_addr,
                  const std::string& lookupd_http_url)
        :_client(&loop, gslam_sub.getTopic(), "gslam", evnsq::Option()),_topic(gslam_sub.getTopic()){
        LOG(INFO)<<"Subscriber:"<<gslam_sub.getTopic();
        _client.SetMessageCallback([this](const evnsq::Message* msg){
            Svar msg1=Svar::json(msg->body.ToString());
//            LOG(INFO)<<"Received "<<msg->body.ToString();
            _pub.publish(SvarNSQ::wrap(msg1));
//            messenger.publish(_topic,SvarNSQ::wrap(msg1));
            return 0;
        });

        if (!lookupd_http_url.empty()) {
            _client.ConnectToLookupds(lookupd_http_url+"/lookup?topic="+gslam_sub.getTopic());
        } else {
            _client.ConnectToNSQDs(nsqd_tcp_addr);
        }

        _pub=messenger.advertise<Svar>(gslam_sub.getTopic());
    }

    static std::string channel(){
#ifdef __unix
        return "gslam"+std::to_string(getpid());
#else
        return "gslam";
#endif
    }

    Publisher        _pub;
    std::string      _topic;
    evnsq::Consumer  _client;
};

class PublisherNSQ{
public:
    PublisherNSQ(evpp::EventLoop& loop,Publisher gslam_pub,
                  const std::string& nsqd_tcp_addr,
                  const std::string& lookupd_http_url)
        : _client(&loop,evnsq::Option())
    {
        LOG(INFO)<<"PublisherNSQ:"<<gslam_pub.getTopic();

        _sub=messenger.subscribe(gslam_pub.getTopic(),0,[this](Svar msg){
            if(std::dynamic_pointer_cast<SvarNSQ>(msg.value())) return;
            std::stringstream sst;
            sst<<msg;
            _client.Publish(_sub.getTopic(),sst.str());
        });

        if (!lookupd_http_url.empty()) {
            _client.ConnectToLookupds(lookupd_http_url+"/lookup?topic="+gslam_pub.getTopic());
        } else {
            _client.ConnectToNSQDs(nsqd_tcp_addr);
        }
    }

    Subscriber       _sub;
    evnsq::Producer _client;
};

int run_nsq(Svar config){
    std::string nsqd_tcp_addr=config.arg("nsqd_tcp_addr","127.0.0.1:4150","the nsqd tcp server address");
    std::string lookupd_http_url=config.arg("lookupd_http_url","","the http server address");

    evpp::EventLoop loop;

    std::map<std::string,std::shared_ptr<SubscriberNSQ>> sub_bridges;
    std::map<std::string,std::shared_ptr<PublisherNSQ> > pub_bridges;
    std::set<std::string> ignore_lut={"messenger/newpub","messenger/newsub","messenger/stop"};

    bool shouldStop=false;
    std::mutex                   mutex_queue;
    std::list<GSLAM::Subscriber> sub_queue;
    std::list<GSLAM::Publisher>  pub_queue;
    std::thread::id              ignore_thread_id;

    auto newpub_func=[&](Publisher pub){
        if(std::this_thread::get_id()==ignore_thread_id) return;
        if(ignore_lut.count(pub.getTopic())) return;
        if(pub.getTopic().find('/')!=std::string::npos) return;
        std::unique_lock<std::mutex> lock(mutex_queue);
        pub_queue.push_back(pub);
    };

    auto newsub_func=[&](Subscriber sub){
        if(std::this_thread::get_id()==ignore_thread_id) return;
        if(ignore_lut.count(sub.getTopic())) return;
        if(sub.getTopic().find('/')!=std::string::npos) return;
        std::unique_lock<std::mutex> lock(mutex_queue);
        sub_queue.push_back(sub);
    };

    GSLAM::Subscriber new_pub=messenger.subscribe("messenger/newpub",0,newpub_func);

    GSLAM::Subscriber new_sub=messenger.subscribe("messenger/newsub",0,newsub_func);

    if(config.get("help",false)){
        return config.help();
    }

    std::thread pubsub_thread([&](){
        ignore_thread_id=std::this_thread::get_id();
        GSLAM::Rate rate(1000);
        while(!shouldStop){
            rate.sleep();
            mutex_queue.lock();
            if(pub_queue.size()){
                Publisher pub=pub_queue.front();
                pub_queue.pop_front();
                if(pub_bridges.count(pub.getTopic())) continue;

                pub_bridges[pub.getTopic()]=std::make_shared<PublisherNSQ>(loop,pub,nsqd_tcp_addr,lookupd_http_url);
            }

            if(sub_queue.size()){
                Subscriber sub=sub_queue.front();
                sub_queue.pop_front();
                if(sub_bridges.count(sub.getTopic())) continue;
                sub_bridges[sub.getTopic()]=std::make_shared<SubscriberNSQ>(loop,sub,nsqd_tcp_addr,lookupd_http_url);
            }
            mutex_queue.unlock();
        }
    });

    for(Publisher pub:messenger.getPublishers()){
        newpub_func(pub);
    }

    for(Subscriber sub:messenger.getSubscribers()){
        newsub_func(sub);
    }

    Subscriber subStop=messenger.subscribe("messenger/stop",0,[&](bool b){
        LOG(INFO)<<"Try stop loop";
        return 0;
    });

    loop.Run();
    shouldStop=true;
    pubsub_thread.join();
    return 0;
}

GSLAM_REGISTER_APPLICATION(nsq,run_nsq);

