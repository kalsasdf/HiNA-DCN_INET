//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "ABMQueue.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/PacketEventTag.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"

namespace inet {

Define_Module(ABMQueue);

int ABMQueue::sharedBuffer[100]={};
uint32_t ABMQueue::congestedNum[100][11]={};


void ABMQueue::initialize(int stage)
{
    REDPFCQueue::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {EV<<"ABMQueue::initialize stage = "<<stage<<endl;
        switchid=findContainingNode(this)->getId();EV<<"switchid = "<<switchid<<endl;
        sharedBuffer[switchid] = par("sharedBuffer");
        priority = par("priority");
        alpha=par("alpha");
        count = -1;
    }
}

void ABMQueue::handleMessage(cMessage *message)
{
    auto packet = check_and_cast<Packet *>(message);
    if(ActiveBufferManagement(priority, message, switchid))
    {
        pushPacket(packet, packet->getArrivalGate());
    }
    else{
        dropPacket(packet, QUEUE_OVERFLOW);
    }
}
//
void ABMQueue::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    cNamedObject packetPushStartedDetails("atomicOperationStarted");
    emit(packetPushStartedSignal, packet, &packetPushStartedDetails);
    EV_INFO << "Pushing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.insert(packet);

    if(usePfc){
        if(queue.getByteLength()>=XOFF&&paused==false){
            paused = true;
            auto pck = new Packet("pause");
            auto newtag=pck->addTagIfAbsent<HiTag>();
            newtag->setOp(ETHERNET_PFC_PAUSE);
            newtag->setPriority(priority);
            newtag->setInterfaceId(eth->getInterfaceId());
            emit(pfcPausedSignal,pck);
            delete pck;

        }
        if(queue.getByteLength()<=XON&&paused==true){
            paused = false;
            auto pck = new Packet("resume");
            auto newtag=pck->addTagIfAbsent<HiTag>();
            newtag->setOp(ETHERNET_PFC_RESUME);
            newtag->setPriority(priority);
            newtag->setInterfaceId(eth->getInterfaceId());
            emit(pfcPausedSignal,pck);
            delete pck;
        }
    }

    if (collector != nullptr && getNumPackets() != 0)
        collector->handleCanPullPacketChanged(outputGate->getPathEndGate());
    cNamedObject packetPushEndedDetails("atomicOperationEnded");
    emit(packetPushEndedSignal, nullptr, &packetPushEndedDetails);
    updateDisplayString();
}

bool ABMQueue::ActiveBufferManagement(uint32_t priority, cMessage *msg, uint32_t switchid){

    Packet *packet = check_and_cast<Packet*>(msg);
    int64_t queueLength = queue.getByteLength();
    HiEthernetMac *radioModule = check_and_cast<HiEthernetMac*>(getParentModule() -> getParentModule() -> getSubmodule("mac"));
    DeqRate = radioModule->pribitpersec[priority];
    if(!isOverloaded())
    {
        return true;
    }
    // 第一个RTT，alpha值取1024，确保吸收Incast
    // 目前启用了两种alpha，数据包优先级为0，alpha为16/63，CNP、Pause报文优先级为99，alpha为32/63
//    if(packet->getifFirstRTT())
//        alpha = 1024;
//    else if(priority <= 11){
//        alpha = alphas[priority];
//    }
    EV << "alpha = " << alpha << endl;
    double RemainingBufferSize = sharedBuffer[switchid];  // 当前交换机共享缓存剩余大小
    EV << "currentSize is " << queueLength << ", RemainingBufferSize " << sharedBuffer[switchid] << ", Packet Length is" << packet->getByteLength() <<endl;
    uint32_t numberofP = congestedNum[switchid][priority] ? congestedNum[switchid][priority] : 1 ;
    uint64_t maxSize = double(alpha*(RemainingBufferSize)/numberofP) * DeqRate ;
    EV << "Threshold is " <<  maxSize << ", congestedNum is "  << congestedNum[switchid][priority] <<  ", congestion state is " << congested << ", DeqRate = " << DeqRate << endl;
    if((queueLength + packet->getByteLength() > (0.9 * maxSize)) && !congested){
        congestedNum[switchid][priority] += 1;
        congested = true;
    }else if(queueLength + packet->getByteLength() <= (0.9 * maxSize) && congestedNum[switchid][priority] > 0 && congested){
        congestedNum[switchid][priority] -= 1;
        congested = false;
    }

    if (((queueLength + packet->getByteLength()) >  maxSize) || (sharedBuffer[switchid] < packet->getByteLength())){
        EV << "it's false" <<endl;
        return false; // drop
    }
    else{
        sharedBuffer[switchid] = RemainingBufferSize - packet->getByteLength();
        return true;
    }
}

} // namespace inet

