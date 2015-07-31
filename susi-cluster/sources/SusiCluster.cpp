#include "susi/SusiCluster.h"

SusiCluster::SusiCluster(std::string addr,short port, std::string key, std::string cert) :
  _susi{new Susi::SusiClient{addr,port,key,cert}} {

}

void SusiCluster::join(){
    _susi->join();
}

void SusiCluster::registerNode(std::string id, std::string addr,short port, std::string key, std::string cert){
    _nodes[id] = std::make_shared<Susi::SusiClient>(addr,port,key,cert);
}

bool SusiCluster::forwardProcessorEvent(std::string topic, std::string id){
    if(_nodes.count(id)==0){
        return false;
    }
    auto & node = _nodes[id];
    _susi->registerProcessor(topic,[node,this](Susi::EventPtr event){
        struct FinishCallback {
            Susi::EventPtr localEvent;
            FinishCallback(Susi::EventPtr evt) : 
                localEvent{std::move(evt)} {}
            FinishCallback(FinishCallback && other) : 
                localEvent{std::move(other.localEvent)} {}
            FinishCallback(FinishCallback & other) : 
                localEvent{std::move(other.localEvent)} {}
            void operator()(Susi::SharedEventPtr remoteEvent){
                *localEvent = *remoteEvent;
            }
        };
        auto remoteEvent = node->createEvent(event->topic);
        *remoteEvent = *event;
        node->publish(std::move(remoteEvent),FinishCallback{std::move(event)});
    }); 
    return true;
}

bool SusiCluster::forwardConsumerEvent(std::string topic, std::string id){
    if(_nodes.count(id)==0){
        return false;
    }
    auto & node = _nodes[id];
    _susi->registerConsumer(topic,[node,id](Susi::SharedEventPtr event){
        auto remoteEvent = node->createEvent(event->topic);
        *remoteEvent = *event;
        std::cout<<"forward consumer event to node "<<id<<std::endl;
        node->publish(std::move(remoteEvent));
    }); 
    return true;
}