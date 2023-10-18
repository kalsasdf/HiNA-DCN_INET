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

    double DeqRate;
    bool congested = false;
    static uint32_t congestedNum[100][11];


  protected:
    virtual ~ABMQueue() { delete packetDropperFunction;  }

    virtual bool BufferManagement(cMessage *msg) override;
};

} // namespace inet

#endif

