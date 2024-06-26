//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_HICLASSIFIER_H
#define __INET_HICLASSIFIER_H

#include <string.h>
#include "../../../../queueing/base/PacketClassifierBase.h"
#include "../../../../queueing/contract/IPacketCollection.h"
#include "../../../../common/packet/Packet.h"
#include "../../../Messages/HiTag/HiTag_m.h"

using namespace inet::queueing;

namespace inet {


class INET_API HiClassifier : public PacketClassifierBase
{
  protected:
    int numOutGates = 0;
    std::map<int, uint32_t> priorityToGateIndexMap;

    int numRcvd = 0;

    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual int classifyPacket(Packet *packet) override;
};


} // namespace inet

#endif

