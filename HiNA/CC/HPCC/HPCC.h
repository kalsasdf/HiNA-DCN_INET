// Copyright (C)
/*
 * Developed by Angrydudu-gy
 * Begin at 2020/04/28
*/
#ifndef __INET_HPCC_H
#define __INET_HPCC_H

#include "../ccheaders.h"
#include "inet/HiNA/Messages/HPCC/INTHeader_m.h"
using namespace std;

namespace inet {

class HPCC : public cSimpleModule
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
    };

    // configuration for .ned file
    bool activate;
    bool isFirstAck = true;
    uint64_t packetid = 0;
    uint64_t nxtSendpacketid = 0;
    double linkspeed;
    int64_t max_pck_size;
    double yita = 0.95;
    int maxstage = 5;
    int lastUpdateSeq = -1;//sequence number of ACK(wanted sequence at the receiver) that triggered the last update
    int incstage = 0;//the count of Addictive Increase
    int expectedFlows;// the expected maximum number of concurrent flows on a link
    double U;//the normalized total inflight bytes
    int send_window;//parameter w
    int csend_window;//parameter wc
    simtime_t baseRTT;
    double currentRate;
    hopInf Last[10];
    int wint;//parameter wint
    int wai;//additive increase(AI) part to ensure fairness

    simtime_t avgDelay1;
    simtime_t avgDelay2;
    simtime_t avgDelay3;
    simtime_t avgDelay4;
    int64_t numNodes[4];
    simtime_t totalDelay[4];

    const char *packetName = "HPCCData";
    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    std::map<uint64_t, sender_packetinfo> sender_packetMap;
    std::map<uint64_t, sender_packetinfo>::iterator iter;

    std::map<L3Address, int> receiver_packetMap;

    TimerMsg *senddata = nullptr;
    simtime_t stopTime;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg);
    virtual void refreshDisplay() const override;
    virtual ~HPCC() {cancelEvent(senddata); delete senddata;}

    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_data(int packetid);

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

    virtual double minval(b numa, b numb);//pick the minimal value

    virtual void receiveAck(Packet *pck);

    virtual void finish() override;
};

} // namespace inet

#endif // ifndef __INET_HPCC_H

