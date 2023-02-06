//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/common/ModuleAccess.h"
#include "inet/common/PacketEventTag.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
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
    if (buffer != nullptr) {
        queue.remove(packet);
        buffer->removePacket(packet);
    }
    else
        queue.pop();

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

