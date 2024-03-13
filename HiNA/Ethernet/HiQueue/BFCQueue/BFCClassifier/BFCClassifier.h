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

#include "../../HiClassifier/HiClassifier.h"

#include <limits>
#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "../../../../../common/ProtocolTag_m.h"
#include "../../../../../common/Simsignals.h"
#include "../../../../../common/packet/Packet.h"

#include "../BFCqueue/BFCqueue.h"
#include "../../../BFCMac/BFCMac.h"
#include "../../../HiQueue/REDPFCQueue/REDPFCQueue.h"
#include "../../../HiQueue/ABMQueue/ABMQueue.h"
#include "../../../HiQueue/HiScheduler/WrrScheduler.h"
#include "../../../../Messages/HiTag/HiTag_m.h"
#include "../../../../Messages/PfcFrame/EthernetPfcFrame_m.h"
#include "../../../../Messages/HPCC/INTHeader_m.h"


#include "../../../../../linklayer/common/EtherType_m.h"
#include "../../../../../linklayer/common/InterfaceTag_m.h"
#include "../../../../../linklayer/common/MacAddressTag_m.h"
#include "../../../../../linklayer/ethernet/common/EthernetControlFrame_m.h"
#include "../../../../../linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "../../../../../linklayer/ethernet/base/EthernetMacBase.h"

#include "../../../../../networklayer/common/NetworkInterface.h"
#include "../../../../../networklayer/ipv4/Ipv4Header_m.h"

#include "../../../../../physicallayer/wired/ethernet/EthernetSignal_m.h"

#include "../../../../../transportlayer/udp/UdpHeader_m.h"



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




