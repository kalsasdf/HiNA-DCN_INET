//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_PCNQUEUE_H
#define __INET_PCNQUEUE_H

#include "../REDPFCQueue/REDPFCQueue.h"

namespace inet {
using namespace inet::queueing;

class INET_API PCNQueue : public REDPFCQueue
{
    int PN;
  protected:
    virtual void initialize(int stage) override;

  public:
    virtual ~PCNQueue() {  }
    virtual Packet *pullPacket(cGate *gate) override;
    virtual void pfcpaused() override;

};

} // namespace inet

#endif

