//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_ABMQUEUE_H
#define __INET_ABMQUEUE_H

#include "inet/HiNA/Ethernet/HiEthernetMac/HiEthernetMac.h"
#include "../REDPFCQueue/REDPFCQueue.h"

namespace inet {
using namespace inet::queueing;

class INET_API ABMQueue : public REDPFCQueue
{
  protected:

    int switchid;
    double DeqRate;
    double alpha;
    bool congested;
    static int sharedBuffer[100];
    static uint32_t congestedNum[100][11];


  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual ~ABMQueue() {  }

    virtual void pushPacket(Packet *packet, cGate *gate) override;
    bool ActiveBufferManagement(uint32_t priority, cMessage *msg, uint32_t switchid);
};

} // namespace inet

#endif

