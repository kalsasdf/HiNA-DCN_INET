// Copyright (C)
/*
 * Developed by HDH
 * Begin at 2023/04/27
*/
#ifndef __INET_POSEIDON_H
#define __INET_POSEIDON_H

#include "../ccheaders.h"
#include "inet/HiNA/Messages/POSEIDON/PSDINTHeader_m.h"
#include<cmath>
using namespace std;

namespace inet {

class POSEIDON : public TransportProtocolBase
{
protected:
    // states for self-scheduling
    enum SenderState{
        SENDING,
        PAUSING,
        STOPPING,
    };
    SenderState SenderState=STOPPING;

    enum SelfpckKindValues {
        SENDDATA,
        TIMEOUT
    };

    struct sender_packetinfo{
        L3Address srcAddr;//ip address
        L3Address destAddr;
        int flowid;
        int srcPort;
        int destPort;
        int length;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        simtime_t cretime;
        bool last = false;
    };

    TimerMsg *senddata = nullptr;
    TimerMsg *timeout = nullptr;
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    int64_t max_pck_size;
    simtime_t baseRTT;
    double PARA_P;
    double PARA_K;
    double m;
    double min_md;


    uint64_t packetid = 0;
    uint64_t nxtSendpacketid = 0;
    uint64_t snd_una = 0;
    int retransmit_cnt = 0;
    int RETX_RESET_THRESHOLD = 5;
    bool can_decrease = true;
    simtime_t RTT_S = 0;
    simtime_t RTT_D = 0;
    simtime_t RTO = 0.1;
    double RTO_alpha = 0.125;
    double RTO_beta = 0.25;
    double min_cwnd = 0.0001;
    int num_dupack = 0;
    simtime_t currentRTT = 0;
    simtime_t t_last_decrease = 0;
    simtime_t pacing_delay = 0;

    double snd_cwnd;
    double cwnd_prev;
    double currentRate;
    double max_cwnd;
    double max_rate;
    double min_rate;
    int num_acked;

    cOutVector currentRTTVector;
    cOutVector targetVector;
    cOutVector cwndVector;
    cOutVector currateVector;
    int bitlength;
    simtime_t maxInterval;
    simtime_t lasttime;
    int timeout_num = 0;

    const char *packetName = "PSDData";
    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    std::map<uint64_t, sender_packetinfo> sender_packetMap;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~POSEIDON() {cancelEvent(senddata); delete senddata;
    cancelEvent(timeout); delete timeout;}

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
    /*
     * Receive XXX, and what should TODO next?
     */

    virtual void receive_data(Packet *pck);

    virtual void receiveAck(Packet *pck);

    virtual void time_out();

    virtual void finish() override;
    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};
};

} // namespace inet

#endif // ifndef __INET_POSEIDON_H

