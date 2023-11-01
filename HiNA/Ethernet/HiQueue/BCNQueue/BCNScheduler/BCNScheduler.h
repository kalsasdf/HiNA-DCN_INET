/*
 * BCNClassifier.h
 *
 *  Created on: 2023年10月23日
 *      Author: ergeng2001
 */


//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_BCNSCHEDULER_H
#define __INET_BCNSCHEDULER_H

#include "inet/queueing/base/PacketSchedulerBase.h"
#include "inet/queueing/contract/IPacketCollection.h"
#include "inet/HiNA/Ethernet/HiEthernetMac/HiEthernetMac.h"
#include "inet/HiNA/Ethernet/BCNMac/BCNMac.h"
#include "inet/HiNA/Messages/BfcFrame/EthernetBfcFrame_m.h"
#include "inet/HiNA/Ethernet/HiQueue/BCNQueue/BCNqueue/BCNqueue.h"

namespace inet {
using namespace inet::queueing;

class INET_API BCNScheduler : public PacketSchedulerBase, public virtual IPacketCollection, public cListener
{
  protected:

    std::vector<BCNqueue *> collections;

    //每个流收到暂停帧的数量；
    std::map<int,int> ispaused;//first is flowid; second is this flowid paused numbers

    int numInputs;


    int last_index = -1;
    int this_index = -1;
    uint64_t last_flowid = -1;
    uint64_t this_flowid = -1;


  protected:
    unsigned int *weights02 = nullptr; // array of weights01 (has numInputs elements)
    unsigned int *buckets02 = nullptr; // array of tokens in buckets01 (has numInputs elements)

    virtual void initialize(int stage) override;
    virtual int schedulePacket() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
    void processbfcframe(cObject *obj);

  public:


    virtual ~BCNScheduler();
    virtual int getMaxNumPackets() const override { return -1; }
    virtual int getNumPackets() const override;

    virtual b getMaxTotalLength() const override { return b(-1); }
    virtual b getTotalLength() const override;

    virtual bool isEmpty() const override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) const override;
    virtual void removePacket(Packet *packet) override;
    virtual void removeAllPackets() override;
    virtual bool canPullSomePacket(cGate *gate) const override;

    virtual Packet *pullPacket(cGate *gate) override;
};

} // namespace inet

#endif










