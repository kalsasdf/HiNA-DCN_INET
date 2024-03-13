//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_MACQUEUE_H
#define __INET_MACQUEUE_H

#include "../HiQueue/REDPFCQueue/REDPFCQueue.h"

using namespace inet::queueing;

namespace inet {

class INET_API SwitchMacQueue : public PacketQueueBase, public IPacketBuffer::ICallback, public IActivePacketSink
{
  protected:
    int packetCapacity = -1;
    b dataCapacity = b(-1);

    IActivePacketSource *producer = nullptr;
    IActivePacketSink *collector = nullptr;
    IPassivePacketSource *provider = nullptr;

    cPacketQueue queue;

  protected:
    virtual void initialize(int stage) override;

  public:
    virtual ~SwitchMacQueue() { }

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
    virtual bool canPullSomePacket(cGate *gate) const override;
    virtual Packet *canPullPacket(cGate *gate) const override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *pullPacket(cGate *gate) override;

    virtual Packet *dequeuePacket() override;

    virtual void handlePacketRemoved(Packet *packet) override;

    virtual void finish() override;

    //IActivePacketSink
    virtual IPassivePacketSource *getProvider(cGate *gate) override { return provider; }
    virtual void handleCanPullPacketChanged(cGate *gate) override;
    virtual void handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) override;
};

} // namespace inet

#endif

