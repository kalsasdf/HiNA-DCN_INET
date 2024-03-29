//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_REDPFCQUEUE_H
#define __INET_REDPFCQUEUE_H

#include "../../../../queueing/base/PacketQueueBase.h"
#include "../../../../queueing/contract/IActivePacketSink.h"
#include "../../../../queueing/contract/IActivePacketSource.h"
#include "../../../../queueing/contract/IPacketBuffer.h"
#include "../../../../queueing/contract/IPacketComparatorFunction.h"
#include "../../../../queueing/contract/IPacketDropperFunction.h"
#include "../../../../queueing/function/PacketComparatorFunction.h"
#include "../../../../queueing/function/PacketDropperFunction.h"
#include "../../../../queueing/marker/EcnMarker.h"

#include "../../../../linklayer/common/FcsMode_m.h"
#include "../../../../linklayer/common/InterfaceTag_m.h"
#include "../../../../linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "../../../../linklayer/ethernet/common/EthernetControlFrame_m.h"

#include "../../../../networklayer/common/NetworkInterface.h"

#include "../../../../common/ModuleAccess.h"
#include "../../../../common/PacketEventTag.h"
#include "../../../../common/Simsignals.h"
#include "../../../../common/TimeTag.h"
#include "../../../../common/IProtocolRegistrationListener.h"

#include "../../../Messages/HiTag/HiTag_m.h"
#include "../../../Messages/PfcFrame/EthernetPfcFrame_m.h"
#include "../../../Ethernet/HiEthernetMac/HiEthernetMac.h"

namespace inet {
using namespace inet::queueing;
using namespace std;

class INET_API REDPFCQueue : public PacketQueueBase, public IPacketBuffer::ICallback
{
  public:
    int packetCapacity = -1;
    b dataCapacity = b(-1);
    static b sharedBuffer[100];
    b headroom;
    double alpha;
    uint64_t maxSize;

    IActivePacketSource *producer = nullptr;
    IActivePacketSink *collector = nullptr;

    cPacketQueue queue;

    IPacketDropperFunction *packetDropperFunction = nullptr;
    IPacketComparatorFunction *packetComparatorFunction = nullptr;

    bool usePfc;
    std::vector<int> paused;
    int XON;
    int XOFF;
    int priority;
    int Kmax;
    int Kmin;
    double Pmax;
    bool useEcn;
    bool markNext = false;
    double count = NaN;
    int switchid;
    int ecncount=0;
    enum RedResult { RANDOMLY_ABOVE_LIMIT, RANDOMLY_BELOW_LIMIT, ABOVE_MAX_LIMIT, BELOW_MIN_LIMIT };
    mutable RedResult lastResult;
    b S_drop;

    cOutVector queuelengthVector;
    cOutVector sharedBufferVector;


  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual IPacketDropperFunction *createDropperFunction(const char *dropperClass) const;
    virtual IPacketComparatorFunction *createComparatorFunction(const char *comparatorClass) const;
    virtual RedResult doRandomEarlyDetection(const Packet *packet);
    virtual bool isOverloaded() const;

  public:
    static simsignal_t pfcPausedSignal;
    static simsignal_t pfcResumeSignal;
    virtual ~REDPFCQueue() { delete packetDropperFunction; }
    virtual void finish() override;

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
    virtual bool canPullSomePacket(cGate *gate) const override { return !isEmpty(); }
    virtual Packet *canPullPacket(cGate *gate) const override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *pullPacket(cGate *gate) override;

    virtual void handlePacketRemoved(Packet *packet) override;

    virtual void pfcpaused() {}
    virtual void pfcresumed() {}
    virtual bool BufferManagement(cMessage *msg);
};

} // namespace inet

#endif

