/*
 * PCN.h
 *
 *  Created on: 20200605
 *      Author: zhouyukun
 */

#ifndef INET_ANGRYDUDU_PCN_PCN_H_
#define INET_ANGRYDUDU_PCN_PCN_H_

#include "../ccheaders.h"
#include"../../Messages/PCN/CNP_m.h"

using namespace std;

namespace inet {

class PCN : public TransportProtocolBase
{
protected:
    enum SenderState{
        SENDING,
        STOPPING
    };
    SenderState SenderState=STOPPING;

    enum SelfpckKindValues {
        SENDDATA,
        CNPTIMER
    };

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

        double currentRate;
        double omega;
    };

    struct receiver_flowinfo{
        simtime_t intervaltime;
        simtime_t lastPckTime;
        bool ecnPakcetReceived;
        double recRate;
        int recNum;
        int64_t recData;
        int ecnNum;
        uint32_t flowid;
        L3Address Sender_srcAddr;
        L3Address Sender_destAddr;
        TimerMsg *cnptimer;
    };

    TimerMsg *senddata = nullptr;
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    simtime_t min_cnp_interval;
    double omega_min;
    double omega_max;
    int64_t max_pck_size;

    L3Address srcAddr;

    const char *packetName = "PCNData";
    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<uint32_t, sender_flowinfo> sender_flowMap;
    std::map<uint32_t, sender_flowinfo>::iterator iter;

    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<uint32_t, receiver_flowinfo> receiver_flowMap;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~PCN() {cancelEvent(senddata); delete senddata;
    for(auto i: receiver_flowMap){
        cancelEvent(i.second.cnptimer);
        delete i.second.cnptimer;
    }
    }
    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_data();
    virtual void send_cnp(uint32_t flowid);
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
    virtual void receive_cnp(Packet *pck);
    virtual void receive_data(Packet *pck);

    virtual void finish() override;
    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};
};

} // namespace inet



#endif /* INET_ANGRYDUDU_PCN_PCN_H_ */
