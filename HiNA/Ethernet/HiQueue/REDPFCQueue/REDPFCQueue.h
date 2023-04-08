//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_REDPFCQUEUE_H
#define __INET_REDPFCQUEUE_H

#include "inet/queueing/base/PacketQueueBase.h"
#include "inet/queueing/contract/IActivePacketSink.h"
#include "inet/queueing/contract/IActivePacketSource.h"
#include "inet/queueing/contract/IPacketBuffer.h"
#include "inet/queueing/contract/IPacketComparatorFunction.h"
#include "inet/queueing/contract/IPacketDropperFunction.h"

#include "inet/linklayer/common/FcsMode_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/HiNA/Messages/PfcFrame/EthernetPfcFrame_m.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/ethernet/common/EthernetControlFrame_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/queueing/marker/EcnMarker.h"

namespace inet {
using namespace inet::queueing;

class INET_API REDPFCQueue : public PacketQueueBase, public IPacketBuffer::ICallback
{
  public:
    int packetCapacity = -1;
    b dataCapacity = b(-1);
    static b sharedBuffer[100][100];
    b headroom;
    double alpha;
    uint64_t maxSize;

    IActivePacketSource *producer = nullptr;
    IActivePacketSink *collector = nullptr;

    cPacketQueue queue;
    IPacketBuffer *buffer = nullptr;

    IPacketDropperFunction *packetDropperFunction = nullptr;
    IPacketComparatorFunction *packetComparatorFunction = nullptr;

    bool usePfc;
    std::vector<int> paused;
    int XON;
    int XOFF;
    int priority;
    NetworkInterface *eth;
    int Kmax;
    int Kmin;
    double Pmax;
    bool useEcn;
    bool markNext = false;
    double count = NaN;
    int switchid;
    enum RedResult { RANDOMLY_ABOVE_LIMIT, RANDOMLY_BELOW_LIMIT, ABOVE_MAX_LIMIT, BELOW_MIN_LIMIT };
    mutable RedResult lastResult;


  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual IPacketDropperFunction *createDropperFunction(const char *dropperClass) const;
    virtual IPacketComparatorFunction *createComparatorFunction(const char *comparatorClass) const;
    virtual RedResult doRandomEarlyDetection(const Packet *packet);
    virtual bool isOverloaded() const;

  public:
    static simsignal_t pfcPausedSignal;
    static simsignal_t pfcResumeSignal;
    virtual ~REDPFCQueue() { delete packetDropperFunction; }

    virtual int getMaxNumPackets() const override { return packetCapacity; }
    virtual int getNumPackets() const override;

    virtual b getMaxTotalLength() const override { return dataCapacity; }
    virtual b getTotalLength() const override { return b(queue.getBitLength()); }

    virtual bool isEmpty() const override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) const override;
    virtual void removePacket(Packet *packet) override;
    virtual void removeAllPackets() override;

    virtual bool supportsPacketPushing(cGate *gate) const override { return inputGate == gate; }
    virtual bool canPushSomePacket(cGate *gate) const override;
    virtual bool canPushPacket(Packet *packet, cGate *gate) const override;
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPacketPulling(cGate *gate) const override { return outputGate == gate; }
    virtual bool canPullSomePacket(cGate *gate) const override { return !isEmpty(); }
    virtual Packet *canPullPacket(cGate *gate) const override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *pullPacket(cGate *gate) override;

    virtual void handlePacketRemoved(Packet *packet) override;

    virtual void pfcpaused() {}
    virtual void pfcresumed() {}
    virtual bool BufferManagement(cMessage *msg);
};

} // namespace inet

#endif

