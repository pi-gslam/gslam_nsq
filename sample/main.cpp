#include <GSLAM/core/GSLAM.h>

using namespace GSLAM;

int run_nsq_sample(Svar config){
    std::string mode=config.arg<std::string>("mode","sub","The mode name: sub or pub");

    if(config.get("help",false)){
        return config.help();
    }

    if(mode=="sub"){
        Subscriber sub=messenger.subscribe("test",[](Svar msg){
                LOG(INFO)<<"Received message:"<<msg;});
        return Messenger::exec();
    }

    Publisher  pub=messenger.advertise<Svar>("test");

    bool shouldStop=false;
    Subscriber subStop=messenger.subscribe("messenger/stop",0,[&](bool b){
        shouldStop=true;
    });

    Rate rate(1);
    while(!shouldStop){
        rate.sleep();
        pub.publish("Hello gslam_nsq.");
    }
    return 0;
}

GSLAM_REGISTER_APPLICATION(nsq_sample,run_nsq_sample);
