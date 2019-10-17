# gslam_nsg : Internet Messaging Plugin based on [NSQ](https://nsq.io/) 

The [GSLAM](https://github.com/zdzhaoyong/GSLAM) core uses a high performace intra process messaging class [Messenger](https://zdzhaoyong.github.io/GSLAM/messenger.html).

But Messenger does not implemented any inter process or network messaging functional.
This project is a messagging extension plugin to auto support network messaging for GSLAM.

Related Projects:
- GSLAM: https://github.com/zdzhaoyong/GSLAM
- NSQ: https://github.com/nsqio/nsq
- evpp: https://github.com/Qihoo360/evpp

**LIMITATION: The messaging payload only support Json now.**

**WARNING: The topic name contains '/' will be ignored!**

## Compile and Install

Please compile and install GSLAM first: https://github.com/zdzhaoyong/GSLAM

Then, compile with cmake:

```
cd gslam_nsq
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

## Usage

Please install NSQ following the doc: https://nsq.io/deployment/installing.html

## Use Local TCP Server

- Start nsq service with nsqd:

```
nsqd
```

- Run sample node with publisher mode

```
gslam nsq_sample nsq -mode pub
```

- Run sample node with subscriber mode

```
gslam nsq_sample nsq -mode sub
```

## Use http Server

- Start nsq http service:

```
nsqlookupd
```

- Start nsq service with http:

```
nsqd --lookupd-tcp-address=127.0.0.1:4160
```

- Run sample node with publisher mode

```
gslam nsq_sample nsq -mode pub -lookupd_http_url 'http://127.0.0.1:4161'
```

- Run sample node with subscriber mode

```
gslam nsq_sample nsq -mode sub -lookupd_http_url 'http://127.0.0.1:4161'
```

## Manage messaging service with nsqdadmin

- Start admin service :

```
nsqadmin --lookupd-http-address=127.0.0.1:4161
```

- To verify things worked as expected, in a web browser open http://127.0.0.1:4171/ to view the nsqadmin UI and see statistics.


## Publish messaging with curl

Publish a string to topic test:
```
curl -d '"I am from curl"' 'http://127.0.0.1:4151/pub?topic=test'
```


