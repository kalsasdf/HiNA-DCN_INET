//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "ABMQueue.h"

namespace inet {

Define_Module(ABMQueue);

uint32_t ABMQueue::congestedNum[100][11]={};

void ABMQueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        queue.setName("storage");
        producer = findConnectedModule<IActivePacketSource>(inputGate);
        collector = findConnectedModule<IActivePacketSink>(outputGate);
        packetCapacity = par("packetCapacity");
        dataCapacity = b(par("dataCapacity"));
        switchid=findContainingNode(this)->getId();EV<<"switchid = "<<switchid<<endl;
        priority = par("priority");
        sharedBuffer[switchid] = b(par("sharedBuffer"));
        headroom = b(par("headroom"));
        usePfc = par("usePfc");
        XON = par("XON");
        XOFF = par("XOFF");
        Kmax=par("Kmax");
        Kmin=par("Kmin");
        Pmax=par("Pmax");
        useEcn=par("useEcn");
        alpha=par("alpha");
        S_drop=b(par("S_drop"));
        interval = par("Timer");
        queuelengthVector.setName("queuelength (bit)");
        sharedBufferVector.setName("sharedbuffer (bit)");
        count = -1;
        WATCH(ecncount);
        packetComparatorFunction = createComparatorFunction(par("comparatorClass"));
        if (packetComparatorFunction != nullptr)
            queue.setup(packetComparatorFunction);
        packetDropperFunction = createDropperFunction(par("dropperClass"));
    }
    else if (stage == INITSTAGE_QUEUEING) {
        checkPacketOperationSupport(inputGate);
        checkPacketOperationSupport(outputGate);
        if (producer != nullptr)
            producer->handleCanPushPacketChanged(inputGate->getPathStartGate());
    }
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

bool ABMQueue::BufferManagement(cMessage *msg){

    Packet *packet = check_and_cast<Packet*>(msg);
    int64_t queueLength = queue.getBitLength();
    if(simTime()-lasttime>interval){
        HiEthernetMac *radioModule = check_and_cast<HiEthernetMac*>(getParentModule() -> getParentModule() -> getSubmodule("mac"));
        DeqRate = radioModule->deqrate[priority];
        numberofP = congestedNum[switchid][priority] ? congestedNum[switchid][priority] : 1 ;
        lasttime=simTime();
    }
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
    b RemainingBufferSize = sharedBuffer[switchid];  // 当前交换机共享缓存剩余大小
    EV << "currentqueuelength is " << queueLength << "b, RemainingBufferSize " << RemainingBufferSize.get() << "b, Packet Length is" << packet->getByteLength() <<"B"<<endl;

    maxSize = double(alpha*(RemainingBufferSize.get())/numberofP) * DeqRate ;
    EV << "Threshold is " << maxSize << "b, congestedNum is " << congestedNum[switchid][priority] << ", congestion state is " << congested << ", DeqRate = " << DeqRate << endl;
    if((queueLength + packet->getBitLength() > (0.9 * maxSize)) && !congested){
        congestedNum[switchid][priority] += 1;
        congested = true;
    }else if(queueLength + packet->getBitLength() <= (0.9 * maxSize) && congestedNum[switchid][priority] > 0 && congested){
        congestedNum[switchid][priority] -= 1;
        congested = false;
    }

    if (queueLength - dataCapacity.get() + packet->getBitLength() >  maxSize){
        EV << "it's false" <<endl;
        return false; // drop
    }
    else{
        sharedBuffer[switchid] = RemainingBufferSize - b(packet->getBitLength());
        return true;
    }
}

} // namespace inet

