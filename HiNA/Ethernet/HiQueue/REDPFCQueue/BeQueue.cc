//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BeQueue.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/PacketEventTag.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"

namespace inet {

Define_Module(BeQueue);

b BeQueue::sharedBuffer[100][100]={};

void BeQueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        queue.setName("storage");
        producer = findConnectedModule<IActivePacketSource>(inputGate);
        collector = findConnectedModule<IActivePacketSink>(outputGate);
        packetCapacity = par("packetCapacity");
        dataCapacity = b(par("dataCapacity"));
        buffer = findModuleFromPar<IPacketBuffer>(par("bufferModule"), this);
        switchid=findContainingNode(this)->getId();EV<<"switchid = "<<switchid<<endl;
        priority = par("priority");
        sharedBuffer[switchid][priority] = b(par("sharedBuffer"));
        headroom = b(par("headroom"));
        alpha=par("alpha");
        queuelengthVector.setName("queuelength (bit)");
        sharedBufferVector.setName("sharedbuffer (bit)");
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

IPacketDropperFunction *BeQueue::createDropperFunction(const char *dropperClass) const
{
    if (strlen(dropperClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketDropperFunction *>(createOne(dropperClass));
}

IPacketComparatorFunction *BeQueue::createComparatorFunction(const char *comparatorClass) const
{
    if (strlen(comparatorClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketComparatorFunction *>(createOne(comparatorClass));
}

bool BeQueue::isOverloaded() const
{
    return (packetCapacity != -1 && getNumPackets() > packetCapacity) ||
           (dataCapacity != b(-1) && getTotalLength() > dataCapacity);
}

int BeQueue::getNumPackets() const
{
    return queue.getLength();
}

Packet *BeQueue::getPacket(int index) const
{
    if (index < 0 || index >= queue.getLength())
        throw cRuntimeError("index %i out of range", index);
    return check_and_cast<Packet *>(queue.get(index));
}

void BeQueue::handleMessage(cMessage *message)
{
    auto packet = check_and_cast<Packet *>(message);
    if(BufferManagement(message))
    {
        pushPacket(packet, packet->getArrivalGate());
    }
    else{
        dropPacket(packet, QUEUE_OVERFLOW);
    }
}

void BeQueue::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    cNamedObject packetPushStartedDetails("atomicOperationStarted");
    emit(packetPushStartedSignal, packet, &packetPushStartedDetails);
    EV_INFO << "Pushing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.insert(packet);
//    if (buffer != nullptr)
//        buffer->addPacket(packet);
//    else if (packetDropperFunction != nullptr) {
//        while (isOverloaded()) {
//            auto packet = packetDropperFunction->selectPacket(this);
//            EV_INFO << "Dropping packet" << EV_FIELD(packet) << EV_ENDL;
//            queue.remove(packet);
//            dropPacket(packet, QUEUE_OVERFLOW);
//        }
//    }
//    ASSERT(!isOverloaded());
    if (collector != nullptr && getNumPackets() != 0)
        collector->handleCanPullPacketChanged(outputGate->getPathEndGate());
    cNamedObject packetPushEndedDetails("atomicOperationEnded");
    emit(packetPushEndedSignal, nullptr, &packetPushEndedDetails);
    updateDisplayString();
}

Packet *BeQueue::pullPacket(cGate *gate)
{
    Enter_Method("pullPacket");
    auto packet = check_and_cast<Packet *>(queue.front());
    EV_INFO << "Pulling packet" << EV_FIELD(packet) << EV_ENDL;
    if(queue.getBitLength()-dataCapacity.get()>=packet->getBitLength()){
        sharedBuffer[switchid][priority]+=b(packet->getBitLength());
    }else if(queue.getBitLength()>=dataCapacity.get()){
        sharedBuffer[switchid][priority]+=b(queue.getBitLength()-dataCapacity.get());
    }
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
    animatePullPacket(packet, outputGate);
    updateDisplayString();
    return packet;
}

void BeQueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.remove(packet);
    if (buffer != nullptr)
        buffer->removePacket(packet);
    emit(packetRemovedSignal, packet);
    updateDisplayString();
}

void BeQueue::removeAllPackets()
{
    Enter_Method("removeAllPackets");
    EV_INFO << "Removing all packets" << EV_ENDL;
    std::vector<Packet *> packets;
    for (int i = 0; i < getNumPackets(); i++)
        packets.push_back(check_and_cast<Packet *>(queue.pop()));
    if (buffer != nullptr)
        buffer->removeAllPackets();
    for (auto packet : packets) {
        emit(packetRemovedSignal, packet);
        delete packet;
    }
    updateDisplayString();
}

bool BeQueue::canPushSomePacket(cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getTotalLength() >= getMaxTotalLength())
        return false;
    return true;
}

bool BeQueue::canPushPacket(Packet *packet, cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getMaxTotalLength() - getTotalLength() < packet->getDataLength())
        return false;
    return true;
}

void BeQueue::handlePacketRemoved(Packet *packet)
{
    Enter_Method("handlePacketRemoved");
    if (queue.contains(packet)) {
        EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
        queue.remove(packet);
        emit(packetRemovedSignal, packet);
        updateDisplayString();
    }
}

bool BeQueue::BufferManagement(cMessage *msg){
    Packet *packet = check_and_cast<Packet*>(msg);
    int64_t queueLength = queue.getBitLength();
    queuelengthVector.recordWithTimestamp(simTime(), queueLength);


    if(!isOverloaded())
    {
        return true;
    }

    b RemainingBufferSize = sharedBuffer[switchid][priority];
    sharedBufferVector.recordWithTimestamp(simTime(), RemainingBufferSize.get());
    EV << "currentqueuelength = " << queueLength << "b, RemainingBufferSize  = " << RemainingBufferSize.get() << "b, Packet Length is" << packet->getByteLength() <<"B"<<endl;

    maxSize = double(alpha*RemainingBufferSize.get()) ;
    if (queueLength + packet->getBitLength() >  maxSize){
        EV << "it's false" <<endl;
        return false; // drop
    }
    else{
        sharedBuffer[switchid][priority] = RemainingBufferSize - b(packet->getBitLength());
        return true;
    }
}

} // namespace inet

