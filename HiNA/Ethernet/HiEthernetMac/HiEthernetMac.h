//
// Copyright (C) 2006 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_ETHERNETMAC_H
#define __INET_ETHERNETMAC_H

#include "inet/linklayer/ethernet/base/EthernetMacBase.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/HiNA/Messages/PfcFrame/EthernetPfcFrame_m.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/HiNA/Messages/HPCC/INTHeader_m.h"
#include "inet/HiNA/Messages/POSEIDON/PSDINTHeader_m.h"
#include "inet/HiNA/Ethernet/HiQueue/HiScheduler/WrrScheduler.h"

namespace inet {

/**
 * A simplified version of EthernetCsmaMac. Since modern Ethernets typically
 * operate over duplex links where's no contention, the original CSMA/CD
 * algorithm is no longer needed. This simplified implementation doesn't
 * contain CSMA/CD, frames are just simply queued up and sent out one by one.
 */
class INET_API HiEthernetMac : public EthernetMacBase
{
  public:
    HiEthernetMac();

    static simsignal_t pfcPausedFrame;
    static simsignal_t pfcResumeFrame;
    simtime_t startTime;
    unsigned int batchSize;    // number of packets in a batch
    simtime_t maxInterval;    // max length of measurement interval (measurement ends
    // if either batchSize or maxInterval is reached, whichever
    // is reached first)


    // global statistics
    unsigned long numPackets;
    unsigned long numBits;

    // current measurement interval
    simtime_t intvlStartTime;
    unsigned long intvlNumPackets;
    unsigned long intvlNumBits;
    unsigned long intvlPriBits[11];
    unsigned long pribitpersec[11];

    // statistics
    cOutVector bitpersecVector;
    cOutVector pkpersecVector;
    int sendpause = 0;
    int sendresume = 0;
    int receivepause = 0;
    int receiveresume = 0;

    //for TIMELY
    bool TIMELY;
    //for TIMELY

    //for HPCC
    bool HPCC;
    uint64_t txBytes = 0;
    //for HPCC

    //for PSD
    bool PSD;
    //for PSD

    //for CoRe
    bool CoRe;
    //for CoRe

    double stopTime;

    // IActivePacketSink:
    virtual void handleCanPullPacketChanged(cGate *gate) override;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void initializeStatistics() override;
    virtual void initializeFlags() override;
    virtual void handleMessageWhenUp(cMessage *msg) override;

    // finish
    virtual void finish() override;

    // event handlers
    virtual void handleEndIFGPeriod();
    virtual void handleEndTxPeriod();
    virtual void handleEndPausePeriod();
    virtual void handleSelfMessage(cMessage *msg) override;

    // helpers
    virtual void startFrameTransmission();
    virtual void handleUpperPacket(Packet *pk) override;
    virtual void processMsgFromNetwork(EthernetSignalBase *signal);
    virtual void processReceivedDataFrame(Packet *packet, const Ptr<const EthernetMacHeader>& frame);
    virtual void processPauseCommand(int pauseUnits);
    virtual void scheduleEndIFGPeriod();
    virtual void scheduleEndPausePeriod(int pauseUnits);
    virtual void beginSendFrames();

    virtual void updateStats(simtime_t now, Packet *pck);
    virtual void beginNewInterval(simtime_t now);

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    void processpfccommand(cObject *obj);
    // statistics
    simtime_t totalSuccessfulRxTime; // total duration of successful transmissions on channel
};

} // namespace inet

#endif

