// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#ifndef __INET_BCN_H
#define __INET_BCN_H

#include "../ccheaders.h"
#include "inet/HiNA/Messages/BFCHeader/BFCHeader_m.h"
#include "inet/HiNA/Messages/BFCHeader/BFCHeaderSerializer.h"

using namespace std;

namespace inet {

class BCN : public TransportProtocolBase
{
public:
    //for statistic
    int receivecnpdel =0;
    int receivecnpspeorigin =0;
    int receivecnpspeup =0;


    enum SenderState{
        SENDING,
        STOPPING
    };
    SenderState SenderState=STOPPING;

    enum SenderAcceleratingState{
        Normal, //0
        Fast_Recovery,  //1
        Additive_Increase,//2
        Hyper_Increase //3
    };

    enum SelfpckKindValues {
        SENDDATA,
        ALPHATIMER,
        RATETIMER,
    };

    map<uint32_t,simtime_t> lastCnpTime; //flowid_simtime()

    struct sender_flowinfo{
        L3Address destAddr;
        uint flowid;
        uint queueid;
        uint pckseq;
        int srcPort;
        int destPort;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        simtime_t cretime;
        int64_t remainLength;
        // for record flowsize
        uint64_t flowsize;

        //为每条流记录的加速状态
        SenderAcceleratingState SenderAcceleratingState = Normal;
//        TimerMsg *rateTimer;
//        TimerMsg *alphaTimer;
        int ByteCounter = 0;
        double currentRate;
        double targetRate;
        double flow_maxTxRate;
        double alpha = 1;
        int iRhai = 0;
        int ByteFrSteps = 0;
        int TimeFrSteps = 0;
    };


    TimerMsg *senddata;
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    double initialrate;
    double gamma;
    simtime_t min_cnp_interval;
    simtime_t AlphaTimer_th;
    simtime_t RateTimer_th;
    double Rai;
    double Rhai;
    int64_t max_pck_size;
    int ByteCounter_th;
    int frSteps_th;
    double maxTxRate;

    L3Address srcAddr;

    const char *packetName = "BCN";
    uint last_flowid = -1;
    simtime_t last_creation_time = 0;

    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    // The destinations addresses of the flows at sender, The flow information at sender
    map<uint32_t, sender_flowinfo> sender_flowMap;
    std::map<uint32_t, sender_flowinfo>::iterator iter;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~BCN() {cancelEvent(senddata); delete senddata;
//    for(auto it = sender_flowMap.begin();it!=sender_flowMap.end();it++){
//    cancelEvent(it->second.rateTimer);
//    delete it->second.rateTimer;
//    cancelEvent(it->second.alphaTimer);
//    delete it->second.alphaTimer;
//    }
    }

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

    virtual void receive_bfccnp(Packet *pck);
    virtual void receive_data(Packet *pck);
    /*
     * rate increase at the sender

    virtual void increaseTxRate(uint32_t flowid);

    virtual void updateAlpha(uint32_t flowid);
    */
    virtual void finish() override;
    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};

};

} // namespace inet

#endif // ifndef __INET_DCQCN_H

