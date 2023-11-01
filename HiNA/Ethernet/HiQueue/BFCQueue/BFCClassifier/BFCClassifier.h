/*
 * BFCClassifier.h
 *
 *  Created on: 2023Äê10ÔÂ22ÈÕ
 *      Author: ergeng2001
 */

//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_BFCCLASSIFIER_H
#define __INET_BFCCLASSIFIER_H

#include "inet/queueing/base/PacketClassifierBase.h"
#include "inet/queueing/contract/IPacketCollection.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/HiNA/Ethernet/BFCMac/BFCMac.h"
#include <string.h>
#include <limits>
using namespace inet::queueing;

namespace inet {


class INET_API BFCClassifier : public PacketClassifierBase
{
  protected:
    int numOutGates = 0;
    std::map<int, uint32_t> numbToGateIndexMap;

    int numRcvd = 0;

    //for BFC£¬map<flowid, queueid>
    std::map<int,int> flowid_queue;
    std::map<int,int>::iterator fq_Iter;

    static simsignal_t packetClassifiedSignal;

    int64_t minlength = std::numeric_limits<int64_t>::max();
    int minqueueid =0;

    int64_t lastflowid = -1;
    int lastqueueid =-1;
    int tempqueueid =-1;


    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual int classifyPacket(Packet *packet) override;
};

} // namespace inet

#endif




