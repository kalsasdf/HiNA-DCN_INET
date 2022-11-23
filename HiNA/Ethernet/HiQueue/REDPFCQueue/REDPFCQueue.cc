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
#include "REDPFCQueue.h"

namespace inet {

Define_Module(REDPFCQueue);

simsignal_t REDPFCQueue::pfcPausedSignal =
        cComponent::registerSignal("pfcPaused");
simsignal_t REDPFCQueue::pfcResumeSignal =
        cComponent::registerSignal("pfcResume");

void REDPFCQueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        queue.setName("storage");
        producer = findConnectedModule<IActivePacketSource>(inputGate);
        collector = findConnectedModule<IActivePacketSink>(outputGate);
        packetCapacity = par("packetCapacity");
        dataCapacity = b(par("dataCapacity"));
        priority = par("priority");
        usePfc = par("usePfc");
        XON = par("XON");
        XOFF = par("XOFF");
        Kmax=par("Kmax");
        Kmin=par("Kmin");
        Pmax=par("Pmax");
        useEcn=par("useEcn");
        count = -1;
        cModule *radioModule = getParentModule()->getParentModule();EV<<"parentmodule = "<<radioModule<<endl;
        eth = check_and_cast<NetworkInterface *>(radioModule);EV<<"eth = "<<eth<<endl;
        buffer = findModuleFromPar<IPacketBuffer>(par("bufferModule"), this);
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

IPacketDropperFunction *REDPFCQueue::createDropperFunction(const char *dropperClass) const
{
    if (strlen(dropperClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketDropperFunction *>(createOne(dropperClass));
}

IPacketComparatorFunction *REDPFCQueue::createComparatorFunction(const char *comparatorClass) const
{
    if (strlen(comparatorClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketComparatorFunction *>(createOne(comparatorClass));
}

bool REDPFCQueue::isOverloaded() const
{
    return (packetCapacity != -1 && getNumPackets() > packetCapacity) ||
           (dataCapacity != b(-1) && getTotalLength() > dataCapacity);
}

int REDPFCQueue::getNumPackets() const
{
    return queue.getLength();
}

Packet *REDPFCQueue::getPacket(int index) const
{
    if (index < 0 || index >= queue.getLength())
        throw cRuntimeError("index %i out of range", index);
    return check_and_cast<Packet *>(queue.get(index));
}

void REDPFCQueue::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    cNamedObject packetPushStartedDetails("atomicOperationStarted");
    emit(packetPushStartedSignal, packet, &packetPushStartedDetails);
    EV_INFO << "Pushing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.insert(packet);
    if (buffer != nullptr)
        buffer->addPacket(packet);
    else if (packetDropperFunction != nullptr) {
        while (isOverloaded()) {
            auto packet = packetDropperFunction->selectPacket(this);
            EV_INFO << "Dropping packet" << EV_FIELD(packet) << EV_ENDL;
            queue.remove(packet);
            dropPacket(packet, QUEUE_OVERFLOW);
        }
    }
    ASSERT(!isOverloaded());

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

Packet *REDPFCQueue::pullPacket(cGate *gate)
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
    lastResult = doRandomEarlyDetection(packet);
    switch (lastResult) {
        case RANDOMLY_ABOVE_LIMIT:
        case ABOVE_MAX_LIMIT: {
            if (useEcn) {
                IpEcnCode ecn = EcnMarker::getEcn(packet);EV<<"ecn = "<<ecn<<endl;
                if (ecn != IP_ECN_NOT_ECT) {
                    // if next packet should be marked and it is not
                    if (markNext && ecn != IP_ECN_CE) {EV<<"set ECN"<<endl;
                        EcnMarker::setEcn(packet, IP_ECN_CE);
                        markNext = false;
                    }
                    else {
                        if (ecn == IP_ECN_CE)
                            markNext = true;
                        else{EV<<"set ECN"<<endl;
                            EcnMarker::setEcn(packet, IP_ECN_CE);
                        }

                    }
                }
            }
        }
        case RANDOMLY_BELOW_LIMIT:
        case BELOW_MIN_LIMIT:
            break;
        default:
            throw cRuntimeError("Unknown RED result");
    }
    animatePullPacket(packet, outputGate);
    updateDisplayString();
    return packet;
}

REDPFCQueue::RedResult REDPFCQueue::doRandomEarlyDetection(const Packet *packet)
{
     int64_t queueLength = queue.getByteLength();
    if (Kmin <= queueLength && queueLength < Kmax) {
        count++;
        const double pb = Pmax * (queueLength - Kmin) / (Kmax - Kmin);
        if (dblrand() < pb) {EV<<"RANDOMLY ABOVE LIMIT"<<endl;
            count = 0;
            return RANDOMLY_ABOVE_LIMIT;
        }
        else{EV<<"RANDOMLY BELOW LIMIT"<<endl;
            return RANDOMLY_BELOW_LIMIT;
        }
    }
    else if (queueLength >= Kmax) {EV<<"ABOVE MAX LIMIT"<<endl;
        count = 0;
        return ABOVE_MAX_LIMIT;
    }
    else {
        count = -1;
    }
    EV<<"BELOW MIN LIMIT"<<endl;
    return BELOW_MIN_LIMIT;
}

void REDPFCQueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.remove(packet);
    if (buffer != nullptr)
        buffer->removePacket(packet);
    emit(packetRemovedSignal, packet);
    updateDisplayString();
}

void REDPFCQueue::removeAllPackets()
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

bool REDPFCQueue::canPushSomePacket(cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getTotalLength() >= getMaxTotalLength())
        return false;
    return true;
}

bool REDPFCQueue::canPushPacket(Packet *packet, cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getMaxTotalLength() - getTotalLength() < packet->getDataLength())
        return false;
    return true;
}

void REDPFCQueue::handlePacketRemoved(Packet *packet)
{
    Enter_Method("handlePacketRemoved");
    if (queue.contains(packet)) {
        EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
        queue.remove(packet);
        emit(packetRemovedSignal, packet);
        updateDisplayString();
    }
}

} // namespace inet

