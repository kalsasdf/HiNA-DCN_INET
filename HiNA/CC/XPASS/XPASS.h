/*
 * Harmonia.h
 *
 *  Created on: 2021Äê3ÔÂ1ÈÕ
 *      Author: kalsasdf
 */

#ifndef INET_HINA_CC_XPASS_XPASS_H_
#define INET_HINA_CC_XPASS_XPASS_H_

#include <map>
#include <queue>

#include <stdlib.h>
#include <string.h>

#include "../ccheaders.h"

using namespace std;

namespace inet {

class XPASS : public cSimpleModule
{
  protected:
    enum ReceiverAcceleratingState{
        Normal,
        Fast_Recovery,
        Additive_Increase,
        Hyper_Increase
    };
    enum SenderState{
        CREQ_SENT,
        CREDIT_RECEIVING,
        CSTOP_SENT
    };

    enum ReceiverState{
        CREDIT_STOP,
        CREDIT_SENDING
    };

    cGate *outGate;
    cGate *inGate;
    cGate *upGate;
    cGate *downGate;
    const char *packetName = "XPASSData";
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    int credit_size;
    double targetratio;
    double wmax;
    uint64_t max_pck_size;
    double maxrate;
    double currate;
    double initialrate;
    double wmin;
    long remainSize = 0;
    L3Address srcaddr;
    simtime_t max_idletime;

    // states for self-scheduling
    enum SelfpckKindValues {
        SENDCRED,
        RATETIMER,
        ALPHATIMER,
        SENDSTOP
    };

    typedef struct receiver {
        L3Address destaddr;
        double max_speed;
        double current_speed;
        simtime_t last_Fbtime;
        simtime_t nowRTT;
        int pck_in_rtt;
        int last_received_data_seq;
        int now_received_data_seq;
        long now_send_cdt_seq;
        int sumlost;
        // for ECN-based rate control
        int ecn_in_rtt;
        bool previousincrease;
        int ByteCounter;
        int iRhai;
        int ByteFrSteps;
        int TimeFrSteps;
        simtime_t LastAlphaTimer;
        simtime_t LastRateTimer;
        ReceiverAcceleratingState ReceiverState;
        simtime_t preRTT;
        simtime_t rtt_diff;
        double omega;
        double alpha;
        double normalized_gradient;
        double targetRate;
        double modeflag;
        TimerMsg *alphaTimer = new TimerMsg("alphaTimer");
        TimerMsg *rateTimer = new TimerMsg("rateTimer");
        TimerMsg *sendcredit = new TimerMsg("sendcredit");
    } receiver_flowinfo;

    typedef struct sender{
        uint32_t flowid;
        simtime_t cretime;
        uint64_t flowsize;
        // for UDP header information
        L3Address destaddr;
        int srcPort;
        int destPort;
        CrcMode crcMode;
        uint16_t crc;
    }sender_flowinfo;

    std::map<L3Address, SenderState> sender_StateMap;

    std::map<L3Address, ReceiverState> receiver_StateMap;

    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<uint32_t, sender_flowinfo> sender_flowMap;

    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<L3Address, receiver_flowinfo> receiver_flowMap;

    // for ecn based control;
    double alpha;
    double targetecnratio;
    simtime_t thigh;
    simtime_t tlow;
    simtime_t minRTT;
    bool useECN;
    bool useRTT;
    int frSteps_th;
    double Rai;
    double Rhai;
    double gamma;
    double rtt_beta;
    double RTT_a;
    double ByteCounter_th;
    simtime_t nowRTT;



  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg);
    virtual ~XPASS() {for(auto i:receiver_flowMap){
        delete i.second.alphaTimer;
        delete i.second.rateTimer;}
    }

    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);
    /**
     * Send credit and request.
     */
    virtual void send_credreq(L3Address destaddr);
    virtual void send_credit(L3Address destaddr);
    /**
     * Send data and credit_stop.
     */
    virtual void send_stop(L3Address destaddr);
    /*
     * Process the Packet from upper layer
     */
    virtual void processUpperpck(Packet *pck);
    /*
     * Process the Packet from lower layer
     */
    virtual void processLowerpck(Packet *pck);
    /*
     * Receive the xxx, and what should todo next?
     */
    virtual void receive_credit(Packet *pck);
    virtual void receive_credreq(Packet *pck);
    virtual void receive_stopcred(Packet *pck);
    virtual void receive_data(Packet *pck);

    virtual void increaseTxRate(L3Address destaddr);
    virtual void updateAlpha(L3Address destaddr);

    virtual void finish() override;

    virtual receiver_flowinfo feedback_control(receiver_flowinfo tinfo)
    {

        double nowrate = tinfo.current_speed;
        int sumlost = tinfo.sumlost;
        int sumcredits = tinfo.pck_in_rtt+tinfo.sumlost;
        double w = tinfo.omega;
        double lossratio = double(sumlost)/double(sumcredits);
        if (lossratio<0||lossratio>1)
        {
            lossratio = 1;
        }
        EV << "feedback_control(), sumlost = "<< sumlost << ", sumcredits = "<< sumcredits<<endl;
        EV << "feedback_control(), loss ratio = "<< lossratio <<endl;
        if(lossratio<=targetratio)
        {
            if(tinfo.previousincrease)
            {
                w = (w+wmax)/2;
            }
            nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio);
            tinfo.previousincrease = true;
        }
        else
        {
            nowrate = nowrate*(1-lossratio)*(1+targetratio);
            if (w/2>wmin)
                w = w/2;
            else
                w = wmin;
            tinfo.previousincrease = false;
        }
        tinfo.omega = w;
        nowrate = (nowrate>maxrate) ? maxrate : nowrate;
        tinfo.current_speed = nowrate;
        EV<<"now speed = "<<nowrate<<endl;
        return tinfo;
    };

    virtual receiver_flowinfo ecnbased_control(receiver_flowinfo tinfo)
    {
        double w = tinfo.omega;
        double oldspeed=tinfo.current_speed;
        double nowrate = tinfo.current_speed;
        int sumlost = tinfo.sumlost;
        int sumcredits = tinfo.pck_in_rtt+tinfo.sumlost;
        double ECN_a=tinfo.alpha;
//                double MODEFLAG=tinfo.modeflag;
        double g=gamma;
        double lossratio = double(sumlost)/double(sumcredits);
        double ecnratio = double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt);
        EV<<"lossratio="<<lossratio<<", sumlost = "<<sumlost<<", sumcredits = "<<sumcredits<<endl;
        if (lossratio<0||lossratio>1) lossratio = 1;
        EV << "feedback_control(), nowRTT = "<< tinfo.nowRTT <<",gradient="<<tinfo.normalized_gradient<< ", ecnratio = "<< ecnratio<<",loss ratio="<<lossratio<<endl;
        EV<<"------------------------------"<<endl;
        EV<<"ecn functioning.old ECN_a="<<ECN_a<<",old speed="<<nowrate<<endl;
        if(ecnratio>targetecnratio){

           /* ECN_a=(1-g)*ECN_a+g*ecnratio;
            nowrate=nowrate*(1-(targetecnratio+ECN_a)/2);
            MODEFLAG=0;
            EV<<"after ecn decreasing,new ECN_a="<<ECN_a<<",new speed="<<nowrate<<endl;*/

            ECN_a=(1-g)*ECN_a+g*ecnratio;
            nowrate=nowrate*(1-(targetecnratio+ECN_a)/2);
            tinfo.alpha=ECN_a;
            tinfo.ByteCounter=0;
            tinfo.LastRateTimer=0;
            tinfo.ByteFrSteps=0;
            tinfo.TimeFrSteps=0;
            tinfo.LastAlphaTimer=0;
            tinfo.iRhai=0;
            EV<<"after ecn decreasing,new ECN_a="<<ECN_a<<",new speed="<<nowrate<<endl;
            cancelEvent(tinfo.alphaTimer);
            cancelEvent(tinfo.rateTimer);
            // update alpha
            scheduleAt(simTime()+nowRTT+0.000005,tinfo.alphaTimer);
            // schedule to rate increase event
            scheduleAt(simTime()+nowRTT+0.000005,tinfo.rateTimer);


        }
        else{
           /* if(MODEFLAG<=3){
                ECN_a=(1-g)*ECN_a+g*ecnratio;
                nowrate=nowrate*(1+(targetecnratio+ECN_a)/2);
                MODEFLAG++;
                EV<<"after ecn increasing,new ECN_a="<<ECN_a<<",new speed="<<nowrate<<",MODEFLAG="<<MODEFLAG<<endl;
            }
            else{
                w = (w+wmax)/2;
                nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio);
                EV<<"after xpass increasing,new speed="<<nowrate<<endl;
            }*/
        }
        EV<<"------------------------------"<<endl;
    if(useRTT){
        EV<<"------------------------------"<<endl;
        EV<<"rtt functioning.old lossratio="<<lossratio<<",old speed="<<nowrate<<endl;

        if(tinfo.nowRTT<tlow){
            EV<<"rtt is too low."<<endl;
            lossratio=0;
        }else{
            if(tinfo.nowRTT>thigh){
                EV<<"rtt is too high."<<endl;
                nowrate=nowrate*(1-rtt_beta*(1-thigh/tinfo.nowRTT));
            }
            else{
                if(tinfo.normalized_gradient>0){
                    EV<<"Gradient > 0."<<endl;
                    nowrate=nowrate*(1-rtt_beta*tinfo.normalized_gradient);
                }
                else{
                    EV<<"Gradient <= 0."<<endl;
                    lossratio=0;
                }
            }
        }
        EV<<"new lossratio="<<lossratio<<",new speed="<<nowrate<<endl;
        EV<<"------------------------------"<<endl;
    }

    EV<<"------------------------------"<<endl;
    EV<<"xpass functioning.old lossratio="<<lossratio<<",old speed="<<nowrate<<endl;
    if(lossratio<=targetratio)
    {
        EV<<"xpass increasing phase.oldspeed="<<nowrate<<",w="<<w<<endl;
        if(tinfo.previousincrease)
        {
            w = (w+wmax)/2;
        }
        nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio);
        tinfo.previousincrease = true;
        EV<<"now w="<<w<<endl;
    }
    else
    {
        EV<<"xpass decreasing phase.oldspeed="<<nowrate<<",w="<<w<<endl;
        nowrate = nowrate*(1-lossratio)*(1+targetratio);
        if (w/2>wmin)
            w = w/2;
        else
            w = wmin;
        tinfo.previousincrease = false;
        EV<<"now w="<<w<<endl;
    }
    EV<<"------------------------------"<<endl;
    tinfo.omega = w;
//            nowrate = (nowrate<minrate) ? minrate : nowrate;
    nowrate = (nowrate>maxrate) ? maxrate : nowrate;
//            tinfo.alpha=ECN_a;// for dctcp
//            tinfo.modeflag=MODEFLAG;//for dctcp
    EV<<"oldspeed="<<oldspeed<<",newspeed="<<nowrate<<endl;
    tinfo.current_speed = nowrate;
    tinfo.targetRate=(tinfo.current_speed>oldspeed)?tinfo.current_speed:oldspeed;//for dcqcn
    return tinfo;

    };
};
} // namespace inet



#endif /* INET_HINA_CC_XPASS_XPASS_H_ */
