//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_ABMQUEUE_H
#define __INET_ABMQUEUE_H

#include "../REDPFCQueue/REDPFCQueue.h"

namespace inet {
using namespace inet::queueing;

class INET_API ABMQueue : public REDPFCQueue
{
  protected:

    double DeqRate;
    bool congested = false;
    static uint32_t congestedNum[100][11];
    simtime_t interval;
    simtime_t lasttime = 0;
    uint32_t numberofP = 1;


  protected:
    virtual void initialize(int stage) override;
    virtual ~ABMQueue() { delete packetDropperFunction;  }

    virtual bool BufferManagement(cMessage *msg) override;
};

} // namespace inet

#endif

