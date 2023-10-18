// Copyright (C)
/*
 * Developed by HDH
 * Begin at 2023/6/6
*/

#ifndef INET_HINA_CC_CoRe_CoRe_H
#define INET_HINA_CC_CoRe_CoRe_H

#include <map>
#include <queue>
#include <stdlib.h>
#include <string.h>

#include "../HiNA/CC/ccheaders.h"
#include "../HiNA/Messages/POSEIDON/PSDINTHeader_m.h"

using namespace std;

namespace inet {

class CoRe : public TransportProtocolBase
{
  protected:
    enum SenderState{
        CSTOP_SENT,
        CREDIT_RECEIVING
    };
    enum ReceiverState
    {
        CREDIT_STOP,
        CREDIT_SENDING
    };

    // states for self-scheduling
    enum SelfpckKindValues {
        SENDCRED,
        SENDRESEND,
        SENDSTOP,
    };

    //���շ�����Ϣ��
    struct receiver_flowinfo{
        L3Address destAddr; //Ŀ�ĵ�ַ
        int flowid; //��ID
        bool if_get[10000]; //���Ƿ��յ�״̬��
        double current_speed;
        int now_received_data_seq; //��ǰ�յ��İ����к�
        long now_send_cdt_seq;
        int ecn_in_rtt;
        int pck_in_rtt;
        simtime_t last_update_time;
        ReceiverState ReceiverState;
        TimerMsg *sendresend;
        TimerMsg *sendcredit;
    };

    //���ͷ�����Ϣ��
    struct sender_flowinfo{
        L3Address destAddr; //Ŀ�ĵ�ַ
        int pckseq; //��ǰ���͵İ����к�
        int flowid; //��ID
        int srcPort; //Դ�˿ں�
        int destPort; //Ŀ�Ķ˿ں�
        int priority;
        CrcMode crcMode; //ѭ��������ģʽ
        uint16_t crc; //ѭ��������
        simtime_t cretime; //������ʱ��
        int64_t flowsize; //����С
    };

    TimerMsg *sendstop = new TimerMsg("sendstop");
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    simtime_t baseRTT;
    int64_t max_pck_size;

    double PARA_P;
    double PARA_K;
    double m;
    double max_rate;
    double min_rate=2e4;

    int64_t sendRtt; //�ѷ��͵�Unscheduled���ֳ���
    int64_t unsSize; //Unscheduled�ܲ��ֳ���
    int64_t sSize; //schedule�����ܳ���
    L3Address srcAddr;
    int rttbytes = 0;
    simtime_t timeout = 0.00002;
    simtime_t last_creation_time = 0;
    simtime_t currentRTT;
    simtime_t t_last_decrease = 0;
    bool can_decrease = false;
    int lastflowid = -1;
    double ECN_a=0;
    double g=0.0625;
//    double currentRate;
    simtime_t lasttime=0;
    int bitlength=0;
    int credit_size = 208;//(84-58)*8,58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)

    cOutVector currentRTTVector;
    cOutVector targetVector;
    cOutVector ratioVector;
    cOutVector mpdVector;
    cOutVector currateVector;
    cOutVector ecnVector;
    cOutVector ecnratioVector;
    cOutVector ECNaVector;



    cGate *outGate;
    cGate *inGate;
    cGate *upGate;
    cGate *downGate;
    const char *packetName = "CoReData";

    std::map<L3Address, SenderState> sender_StateMap;

    std::map<L3Address, ReceiverState> receiver_StateMap;

    //���ͷ�����
    std::map<uint32_t, sender_flowinfo> sender_flowMap;

    //���շ�����
    std::map<L3Address, receiver_flowinfo> receiver_flowMap;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~CoRe() {cancelEvent(sendstop); delete sendstop;
//        for(auto i = receiver_flowMap.begin();i != receiver_flowMap.end();i++){
//            cancelEvent(i->second.sendresend);
//            delete i->second.sendresend;
//        }
    }

    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_resend(uint32_t flowid, long seq);
    virtual void send_unschedule(uint32_t flowid);

    virtual void send_credreq(uint32_t flowid);
    virtual void send_credit(L3Address destaddr);
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
    virtual void receive_credit(Packet *pck);
    virtual void receive_credreq(Packet *pck);
    virtual void receive_data(Packet *pck);
    virtual void receive_resend(Packet *pck);

    virtual void send_stop(L3Address destaddr);
    virtual void receive_stopcred(Packet *pck);

    virtual void finish() override;

    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};

};
} // namespace inet

#endif // ifndef __INET_HOMA_H

