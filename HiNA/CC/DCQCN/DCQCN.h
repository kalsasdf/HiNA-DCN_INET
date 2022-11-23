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

class IInterfaceTable;

class DCQCN : public cSimpleModule
{
protected:
    enum SenderState{
        SENDING,
        STOPPING
    };
    SenderState SenderState = STOPPING;

    enum SenderAcceleratingState{
        Normal,
        Fast_Recovery,
        Additive_Increase,
        Hyper_Increase
    };
    SenderAcceleratingState SenderAcceleratingState = Normal;

    enum SelfpckKindValues {
        SENDDATA,
        ALPHATIMER,
        RATETIMER,
    };

    simtime_t lastCnpTime = 0;

    typedef struct sender{
        L3Address destAddr;
        uint32_t flowid;
        int srcPort;
        int destPort;
        int flowsize;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        simtime_t cretime;
    }sender_flowinfo;

    L3Address srcAddr;
    simtime_t nxtSendTime;
    int ByteCounter = 0; // for gaining rate
    int ByteCounter_th;
    double currentRate;
    double targetRate;
    double maxTxRate;
    double alpha = 1;
    simtime_t lasttime;
    int iRhai = 0;
    int ByteFrSteps = 0; // rate have been increased for frSteps times.
    int TimeFrSteps = 0; // rate have been increased for frSteps times.
    int frSteps_th;
    TimerMsg *rateTimer;
    TimerMsg *alphaTimer;

    // configuration for .ned file
    double linkspeed;
    double gamma;
    simtime_t min_cnp_interval;
    simtime_t AlphaTimer_th;
    simtime_t RateTimer_th;
    double Rai;
    double Rhai;

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

    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_data();
    virtual void send_cnp(L3Address destaddr);
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
    virtual void increaseTxRate(L3Address destaddr);

    virtual void updateAlpha();

    virtual void finish() override;
};

} // namespace inet

#endif // ifndef __INET_DCQCN_H

