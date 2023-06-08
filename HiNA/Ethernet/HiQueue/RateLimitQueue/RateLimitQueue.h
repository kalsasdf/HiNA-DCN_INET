//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_RATELIMITQUEUE_H
#define __INET_RATELIMITQUEUE_H

#include "inet/queueing/base/PacketQueueBase.h"
#include "inet/queueing/contract/IActivePacketSink.h"
#include "inet/queueing/contract/IActivePacketSource.h"
#include "inet/queueing/contract/IPacketBuffer.h"
#include "inet/queueing/contract/IPacketComparatorFunction.h"
#include "inet/queueing/contract/IPacketDropperFunction.h"

namespace inet {
using namespace inet::queueing;

class INET_API RateLimitQueue : public PacketQueueBase, public IPacketBuffer::ICallback
{
  protected:
    int packetCapacity = -1;
    b dataCapacity = b(-1);

    IActivePacketSource *producer = nullptr;
    IActivePacketSink *collector = nullptr;

    cPacketQueue queue;
    IPacketBuffer *buffer = nullptr;

    IPacketDropperFunction *packetDropperFunction = nullptr;
    IPacketComparatorFunction *packetComparatorFunction = nullptr;

    double limrate = NaN;
    simtime_t lasttime=0;
    cMessage *canpull = new cMessage;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    virtual IPacketDropperFunction *createDropperFunction(const char *dropperClass) const;
    virtual IPacketComparatorFunction *createComparatorFunction(const char *comparatorClass) const;

    virtual bool isOverloaded() const;

  public:
    virtual ~RateLimitQueue() { delete packetDropperFunction; cancelEvent(canpull);delete canpull;}

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
    virtual bool canPullSomePacket(cGate *gate) const override ;
    virtual Packet *canPullPacket(cGate *gate) const override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *pullPacket(cGate *gate) override;

    virtual void handlePacketRemoved(Packet *packet) override;
};

} // namespace inet

#endif

