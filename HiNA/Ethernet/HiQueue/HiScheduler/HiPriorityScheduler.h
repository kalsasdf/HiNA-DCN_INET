//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_HIPRIORITYSCHEDULER_H
#define __INET_HIPRIORITYSCHEDULER_H

#include "inet/queueing/base/PacketSchedulerBase.h"
#include "inet/queueing/contract/IPacketCollection.h"
#include "inet/HiNA/Ethernet/HiEthernetMac/HiEthernetMac.h"
#include "../REDPFCQueue/REDPFCQueue.h"

namespace inet {
using namespace inet::queueing;

class INET_API HiPriorityScheduler : public PacketSchedulerBase, public virtual IPacketCollection, public cListener
{
  protected:
    std::vector<REDPFCQueue *> collections;

    std::map<int,bool> ispaused;
    int numInputs;

  protected:
    virtual void initialize(int stage) override;
    virtual int schedulePacket() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
    void processpfcframe(cObject *obj);

  public:
    virtual int getMaxNumPackets() const override { return -1; }
    virtual int getNumPackets() const override;

    virtual b getMaxTotalLength() const override { return b(-1); }
    virtual b getTotalLength() const override;

    virtual bool isEmpty() const override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) const override;
    virtual void removePacket(Packet *packet) override;
    virtual void removeAllPackets() override;
    virtual bool canPullSomePacket(cGate *gate) const override;
};

} // namespace inet

#endif

