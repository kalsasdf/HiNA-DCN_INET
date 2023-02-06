// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#ifndef __INET_DCQCN_H
#define __INET_DCQCN_H

#include "../ccheaders.h"

using namespace std;

namespace inet {

class DCQCN : public cSimpleModule
{
protected:
    enum SenderState{
        SENDING,
        STOPPING
    };
    SenderState SenderState=STOPPING;

    enum SenderAcceleratingState{
        Normal,
        Fast_Recovery,
        Additive_Increase,
        Hyper_Increase
    };


    enum SelfpckKindValues {
        SENDDATA,
        ALPHATIMER,
        RATETIMER,
    };

    map<uint32_t,simtime_t> lastCnpTime;

    struct sender_flowinfo{
        L3Address destAddr;
        uint flowid;
        uint pckseq;
        int srcPort;
        int destPort;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        simtime_t cretime;
        int64_t remainLength;

        SenderAcceleratingState SenderAcceleratingState = Normal;
        TimerMsg *rateTimer;
        TimerMsg *alphaTimer;
        int ByteCounter = 0;
        double currentRate;
        double targetRate;
        double maxTxRate;
        double alpha = 1;
        int iRhai = 0;
        int ByteFrSteps = 0;
        int TimeFrSteps = 0;
    };

    L3Address srcAddr;
    TimerMsg *senddata = nullptr;

    // configuration for .ned file
    double linkspeed;
    double gamma;
    simtime_t min_cnp_interval;
    simtime_t AlphaTimer_th;
    simtime_t RateTimer_th;
    double Rai;
    double Rhai;
    int64_t max_pck_size;
    int ByteCounter_th;
    int frSteps_th;

    const char *packetName = "DcqcnData";

    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    // The destinations addresses of the flows at sender, The flow information at sender
    map<uint32_t, sender_flowinfo> sender_flowMap;


protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg);
    virtual void refreshDisplay() const override;
    virtual ~DCQCN() {cancelEvent(senddata); delete senddata;}

    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_data();
    /*
     * Process the Packet from upper layer
     */
    virtual void processUpperpck(Packet *pck);
    /*
     * Process the Packet from lower layer
     */
    virtual void processLowerpck(Packet *pck);

    virtual void receive_cnp(Packet *pck);
    virtual void receive_data(Packet *pck);
    /*
     * rate increase at the sender
     */
    virtual void increaseTxRate(uint32_t flowid);

    virtual void updateAlpha(uint32_t flowid);

    virtual void finish() override;
};

} // namespace inet

#endif // ifndef __INET_DCQCN_H

