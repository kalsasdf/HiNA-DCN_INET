//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "PCNQueue.h"

namespace inet {

Define_Module(PCNQueue);


void PCNQueue::initialize(int stage)
{
    REDPFCQueue::initialize(stage);
}

Packet *PCNQueue::pullPacket(cGate *gate)
{
    Enter_Method("pullPacket");
    auto packet = check_and_cast<Packet *>(queue.front());
    EV_INFO << "Pulling packet" << EV_FIELD(packet) << EV_ENDL;
    EV<<"queuelength = "<<queue.getBitLength()<<"b"<<endl;
    if(queue.getBitLength()-dataCapacity.get()>=packet->getBitLength()){
        sharedBuffer[switchid]+=b(packet->getBitLength());
    }else if(queue.getBitLength()>=dataCapacity.get()){
        sharedBuffer[switchid]+=b(queue.getBitLength()-dataCapacity.get());
    }
    queue.pop();
    queuelengthVector.recordWithTimestamp(simTime(), queue.getBitLength());
    EV<<"after pop queuelength = "<<queue.getBitLength()<<endl;

    auto queueingTime = simTime() - packet->getArrivalTime();
    auto packetEvent = new PacketQueuedEvent();
    packetEvent->setQueuePacketLength(getNumPackets());
    packetEvent->setQueueDataLength(getTotalLength());
    insertPacketEvent(this, packet, PEK_QUEUED, queueingTime, packetEvent);
    increaseTimeTag<QueueingTimeTag>(packet, queueingTime, queueingTime);
    emit(packetPulledSignal, packet);

    if(PN==0){
        if(queue.getByteLength()>0){
            IpEcnCode ecn = EcnMarker::getEcn(packet);
             if (ecn != IP_ECN_NOT_ECT) {
                 // if next packet should be marked and it is not
                 if (markNext && ecn != IP_ECN_CE) {
                     EcnMarker::setEcn(packet, IP_ECN_CE);
                     markNext = false;
                 }
                 else {
                     if (ecn == IP_ECN_CE)
                         markNext = true;
                     else
                         EcnMarker::setEcn(packet, IP_ECN_CE);
                 }
             }
        }
    }else{
        PN--;
    }
    animatePullPacket(packet, outputGate);
    updateDisplayString();
    return packet;
}

void PCNQueue::pfcpaused()
{
    PN=queue.getLength();
}
} // namespace inet

