//
// Copyright (C) 2006 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "HiEthernetMac.h"

#include "../HiQueue/REDPFCQueue/REDPFCQueue.h"
#include "../HiQueue/ABMQueue/ABMQueue.h"


namespace inet {

// TODO refactor using a statemachine that is present in a single function
// TODO this helps understanding what interactions are there and how they affect the state

Define_Module(HiEthernetMac);

simsignal_t HiEthernetMac::pfcPausedFrame =
        cComponent::registerSignal("pfcpauseframe");
simsignal_t HiEthernetMac::pfcResumeFrame =
        cComponent::registerSignal("pfcresumeframe");

HiEthernetMac::HiEthernetMac()
{
}

void HiEthernetMac::initialize(int stage)
{
    EthernetMacBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

        batchSize = par("batchSize");
        maxInterval = par("maxInterval");

        numPackets = numBits  = 0;
        intvlStartTime = 0;
        intvlNumPackets = intvlNumBits = 0;

        WATCH(numPackets);
        WATCH(numBits);
        WATCH(intvlStartTime);
        WATCH(intvlNumPackets);
        WATCH(intvlNumBits);
        WATCH(sendpause);
        WATCH(sendresume);
        WATCH(receivepause);
        WATCH(receiveresume);

        bitpersecVector.setName("thruput (bps)");
        pkpersecVector.setName("packet/sec");
        cModule *radioModule = getParentModule()->getParentModule();EV<<"parentmodule = "<<radioModule<<endl;
        radioModule->subscribe(REDPFCQueue::pfcPausedSignal,this);
        radioModule->subscribe(REDPFCQueue::pfcResumeSignal,this);

        TIMELY=par("TIMELY");
        HPCC=par("HPCC");
        PSD=par("PSD");
        CoRe=par("CoRe");
        stopTime=par("stopTime");

        if (!par("duplexMode"))
            throw cRuntimeError("Half duplex operation is not supported by HiEthernetMac, use the EthernetCsmaMac module for that! (Please enable csmacdSupport on EthernetInterface)");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        beginSendFrames(); // FIXME choose an another stage for it
    }
}

void HiEthernetMac::initializeStatistics()
{
    EthernetMacBase::initializeStatistics();

    // initialize statistics
    totalSuccessfulRxTime = 0.0;
}

void HiEthernetMac::initializeFlags()
{
    EthernetMacBase::initializeFlags();

    duplexMode = true;
    physInGate->setDeliverImmediately(false);
}

void HiEthernetMac::handleMessageWhenUp(cMessage *msg)
{
    if (channelsDiffer)
        readChannelParameters(true);

    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else if (msg->getArrivalGateId() == upperLayerInGateId)
        handleUpperPacket(check_and_cast<Packet *>(msg));
    else if (msg->getArrivalGate() == physInGate)
        processMsgFromNetwork(check_and_cast<EthernetSignalBase *>(msg));
    else
        throw cRuntimeError("Message received from unknown gate!");
    processAtHandleMessageFinished();
}

void HiEthernetMac::handleSelfMessage(cMessage *msg)
{
    EV_TRACE << "Self-message " << msg << " received\n";

    if (msg == endTxTimer)
        handleEndTxPeriod();
    else if (msg == endIfgTimer)
        handleEndIFGPeriod();
    else if (msg == endPauseTimer)
        handleEndPausePeriod();
    else
        throw cRuntimeError("Unknown self message received!");
}

void HiEthernetMac::startFrameTransmission()
{
    ASSERT(currentTxFrame);
    EV_DETAIL << "Transmitting a copy of frame " << currentTxFrame << endl;

    Packet *frame = currentTxFrame->dup(); // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()
    const auto& hdr = frame->peekAtFront<EthernetMacHeader>(); // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()
    ASSERT(hdr);
    ASSERT(!hdr->getSrc().isUnspecified());


    //for HPCC
    if(HPCC){
        txBytes+=frame->getByteLength();
        if(std::string(frame->getFullName()).find("HPCCData") != std::string::npos){
            auto& eth_hdr = frame->popAtFront<EthernetMacHeader>();//移除mac头
            auto& ip_hdr = frame->popAtFront<Ipv4Header>();//移除ip头
            auto& INT_hdr = frame->removeAtFront<INTHeader>();//移除INT头
            hopInf cur_inf;
            cur_inf.TS= simTime();//记录时间戳
            cur_inf.queueLength = check_and_cast<WrrScheduler*>(getParentModule()->getSubmodule("Hiqueue")->getSubmodule("WrrScheduler"))->getTotalLength();
            cur_inf.txBytes = txBytes;
            cur_inf.txRate = curEtherDescr->txrate;
            EV<<"(mac) the queueLenth is "<<cur_inf.queueLength<<" the txBytes is "<<cur_inf.txBytes<<" the txrate is "<<cur_inf.txRate<<endl;
            int nHop= INT_hdr->getNHop()+1;
            INT_hdr->setNHop(nHop);
            int pathID = INT_hdr->getPathID() ^ hostModule->getId();//hostID未确定
            INT_hdr->setPathID(pathID);
            INT_hdr->setHopInfs(nHop-1,cur_inf);
            frame->insertAtFront(INT_hdr);//加INT头
            frame->insertAtFront(ip_hdr);//加ip头
            frame->insertAtFront(eth_hdr);//加mac头
        }
    }
    //for HPCC

    //for PSD
    if(PSD&&std::string(frame->getFullName()).find("PSDData") != std::string::npos){
        auto& eth_hdr = frame->popAtFront<EthernetMacHeader>();//移除mac头
        auto& ip_hdr = frame->popAtFront<Ipv4Header>();//移除ip头
        auto& INT_hdr = frame->removeAtFront<PSDINTHeader>();//移除INT头
        if(simTime()-INT_hdr->getTS()>INT_hdr->getMPD())
            INT_hdr->setMPD(simTime()-INT_hdr->getTS());
        EV<<"mpd = "<<INT_hdr->getMPD();
        INT_hdr->setTS(simTime());
        frame->insertAtFront(INT_hdr);//加INT头
        frame->insertAtFront(ip_hdr);//加ip头
        frame->insertAtFront(eth_hdr);//加mac头
    }
    //for PSD

    //for CoRe
    if(CoRe&&std::string(frame->getFullName()).find("CoRe") != std::string::npos){
        auto& eth_hdr = frame->popAtFront<EthernetMacHeader>();//移除mac头
        auto& ip_hdr = frame->popAtFront<Ipv4Header>();//移除ip头
        auto& INT_hdr = frame->removeAtFront<PSDINTHeader>();//移除INT头
        if(simTime()-INT_hdr->getTS()>INT_hdr->getMPD())
            INT_hdr->setMPD(simTime()-INT_hdr->getTS());
        EV<<"mpd = "<<INT_hdr->getMPD();
        INT_hdr->setTS(simTime());
        frame->insertAtFront(INT_hdr);//加INT头
        frame->insertAtFront(ip_hdr);//加ip头
        frame->insertAtFront(eth_hdr);//加mac头
    }
    //for CoRe

    // add preamble and SFD (Starting Frame Delimiter), then send out
    encapsulate(frame);
    updateStats(simTime(), frame);//for rate-meter

    //for TIMELY
    if(TIMELY&&std::string(frame->getFullName()).find("TIMELYData") != std::string::npos){
        frame->setTimestamp(simTime());
        EV<<"TIMELY tsend = "<<simTime()<<endl;
    }
    //for TIMELY

    // send
    auto& oldPacketProtocolTag = frame->removeTag<PacketProtocolTag>();
    frame->clearTags();
    auto newPacketProtocolTag = frame->addTag<PacketProtocolTag>();
    *newPacketProtocolTag = *oldPacketProtocolTag;
    EV_INFO << "Transmission of " << frame << " started.\n";
    auto signal = new EthernetSignal(frame->getName());
    signal->setSrcMacFullDuplex(duplexMode);
    signal->setBitrate(curEtherDescr->txrate);
    if (sendRawBytes) {
        auto bytes = frame->peekDataAsBytes();
        frame->eraseAll();
        frame->insertAtFront(bytes);
    }
    signal->encapsulate(frame);

    ASSERT(curTxSignal == nullptr);
    curTxSignal = signal->dup();
    emit(transmissionStartedSignal, signal);
    send(signal, physOutGate);

    scheduleAt(transmissionChannel->getTransmissionFinishTime(), endTxTimer);
    changeTransmissionState(TRANSMITTING_STATE);
}

void HiEthernetMac::updateStats(simtime_t now, Packet *pck)
{
    numPackets++;
    int bits=pck->getBitLength();
    numBits += bits;

    int priority;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        priority = region.getTag()->getPriority();
    }
    if(priority>11||priority<0)
        priority = 10;

    intvlNumPackets++;
    intvlNumBits += bits;
    EV<<"intvlNumbits = "<<intvlNumBits<<", intvlNumPackets = "<<intvlNumPackets<<endl;
    intvlPriBits[priority] += bits;


}

void HiEthernetMac::beginNewInterval(simtime_t now)
{
    simtime_t duration = now - intvlStartTime;
    EV<<"interval = "<<duration<<"s"<<", dbl = "<<duration.dbl()<<"s"<<endl;

    // record measurements
    double bitpersec = intvlNumBits / duration.dbl();
    double pkpersec = intvlNumPackets / duration.dbl();
    EV<<"currate = "<<bitpersec<<"bps "<<pkpersec<<"pps"<<endl;
    for(int i = 0 ; i < 11 ; i++){
        pribitpersec[i] = intvlPriBits[i] / duration.dbl();
        intvlPriBits[i] = 0;
    }

    bitpersecVector.recordWithTimestamp(intvlStartTime, bitpersec);
    pkpersecVector.recordWithTimestamp(intvlStartTime, pkpersec);

    // restart counters
    intvlStartTime = now;    // FIXME this should be *beginning* of tx of this packet, not end!
    intvlNumPackets = intvlNumBits = 0;
}

void HiEthernetMac::handleUpperPacket(Packet *packet)
{
    EV_INFO << "Received " << packet << " from upper layer." << endl;

    if(simTime()>stopTime){
        delete packet;
    }else{
        numFramesFromHL++;
        emit(packetReceivedFromUpperSignal, packet);

        auto frame = packet->peekAtFront<EthernetMacHeader>();
        if (frame->getDest().equals(getMacAddress())) {
            throw cRuntimeError("logic error: frame %s from higher layer has local MAC address as dest (%s)",
                    packet->getFullName(), frame->getDest().str().c_str());
        }

        if (packet->getDataLength() > MAX_ETHERNET_FRAME_BYTES) { // FIXME two MAX FRAME BYTES in specif...
            throw cRuntimeError("packet from higher layer (%s) exceeds maximum Ethernet frame size (%s)",
                    B(packet->getByteLength()).str().c_str(), MAX_ETHERNET_FRAME_BYTES.str().c_str());
        }

        if (!connected) {
            EV_WARN << "Interface is not connected -- dropping packet " << packet << endl;
            PacketDropDetails details;
            details.setReason(INTERFACE_DOWN);
            emit(packetDroppedSignal, packet, &details);
            numDroppedPkFromHLIfaceDown++;
            delete packet;

            return;
        }

        // fill in src address if not set
        if (frame->getSrc().isUnspecified()) {
            frame = nullptr; // drop shared ptr
            auto newFrame = packet->removeAtFront<EthernetMacHeader>();
            newFrame->setSrc(getMacAddress());
            packet->insertAtFront(newFrame);
            frame = newFrame;
        }

        if (transmitState != TX_IDLE_STATE)
            throw cRuntimeError("HiEthernetMac not in TX_IDLE_STATE when packet arrived from upper layer");
        if (currentTxFrame != nullptr)
            throw cRuntimeError("HiEthernetMac already has a transmit packet when packet arrived from upper layer");
        addPaddingAndSetFcs(packet, MIN_ETHERNET_FRAME_BYTES);
        currentTxFrame = packet;
        startFrameTransmission();
    }
}

void HiEthernetMac::processMsgFromNetwork(EthernetSignalBase *signal)
{
    EV_INFO <<"HiEthernetMac::processMsgFromNetwork(), "<< signal << " received." << endl;

    if (!connected) {
        EV_WARN << "Interface is not connected -- dropping signal " << signal << endl;
        if (dynamic_cast<EthernetSignal *>(signal)) { // do not count JAM and IFG packets
            auto packet = check_and_cast<Packet *>(signal->decapsulate());
            delete signal;
            decapsulate(packet);
            PacketDropDetails details;
            details.setReason(INTERFACE_DOWN);
            emit(packetDroppedSignal, packet, &details);
            delete packet;
            numDroppedIfaceDown++;
        }
        else
            delete signal;

        return;
    }

    emit(receptionEndedSignal, signal);

    totalSuccessfulRxTime += signal->getDuration();

    if (signal->getSrcMacFullDuplex() != duplexMode)
        throw cRuntimeError("Ethernet misconfiguration: MACs on the same link must be all in full duplex mode, or all in half-duplex mode");

    if (dynamic_cast<EthernetFilledIfgSignal *>(signal))
        throw cRuntimeError("There is no burst mode in full-duplex operation: EtherFilledIfg is unexpected");

    if (dynamic_cast<EthernetJamSignal *>(signal))
        throw cRuntimeError("There is no JAM signal in full-duplex operation: EthernetJamSignal is unexpected");

    bool hasBitError = signal->hasBitError();
    auto packet = check_and_cast<Packet *>(signal->decapsulate());
    delete signal;
    decapsulate(packet);
    emit(packetReceivedFromLowerSignal, packet);

    if (hasBitError || !verifyCrcAndLength(packet)) {
        EV_WARN << "Bit error/CRC error in incoming packet -- dropping packet " << EV_FIELD(packet) << EV_ENDL;
        numDroppedBitError++;
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }

    //for TIMELY
    if(TIMELY){
        if (std::string(packet->getFullName()).find("TIMELYACK") != std::string::npos)
        {
            simtime_t nowRTT = simTime() - packet->getTimestamp() - packet->getByteLength()*8/ curEtherDescr->txrate;  // 10*1e9 是测试所使用的链路速率
            EV <<"tsend = " << packet->getTimestamp() << ", tcompletion = " << simTime() << ", nowRTT = " << nowRTT <<endl;

            packet->setTimestamp(nowRTT);
        }
        else if((std::string(packet->getFullName()).find("TIMELYData") != std::string::npos))
        {
            bool isLastPck;
            for (auto& region : packet->peekData()->getAllTags<HiTag>()){
                isLastPck = region.getTag()->isLastPck();
            }
            if(isLastPck){
                EV<<"receive an end of seg"<<endl;
                simtime_t tsend = packet->getTimestamp();
                Packet *ack = packet->dup();
                auto ack_eth = ack->removeAtFront<EthernetMacHeader>();
                auto ack_ip = ack->removeAtFront<Ipv4Header>();
                auto ack_udp = ack->removeAtFront<UdpHeader>();

                const auto& payload = makeShared<ByteCountChunk>(B(1));
                auto tag = payload->addTag<HiTag>();
                tag->setPriority(-1);
                Packet *ack1 = new Packet("TIMELYACK",payload);

                ack1->insertAtFront(ack_udp);

                auto ip_srcaddr = ack_ip->getSrcAddress();
                auto ip_destaddr = ack_ip->getDestAddress();
                ack_ip->setDestAddress(ip_srcaddr);
                ack_ip->setSrcAddress(ip_destaddr);
                ack_ip->setTotalLengthField(ack_ip->getChunkLength()+ack1->getDataLength());
                ack1->insertAtFront(ack_ip);

                ack_eth->setSrc(getMacAddress());
                ack_eth->setDest(MacAddress::BROADCAST_ADDRESS);
                ack1->insertAtFront(ack_eth);
                const auto& ethernetFcs = makeShared<EthernetFcs>();
                ethernetFcs->setFcsMode(fcsMode);
                ack1->insertAtBack(ethernetFcs);
                ack1->setTimestamp(tsend);
                addPaddingAndSetFcs(ack1, MIN_ETHERNET_FRAME_BYTES);

                while(currentTxFrame==nullptr&& transmitState == TX_IDLE_STATE){
                    currentTxFrame = ack1;
                    EV_DETAIL << "Send an ACK" << endl;
                    startFrameTransmission();
                }
                delete ack;
            }
        }
    }
    //for TIMELY

    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    if (dropFrameNotForUs(packet, frame))
        return;

    if (frame->getTypeOrLength() == ETHERTYPE_FLOW_CONTROL) {
        const auto& controlFrame = packet->peekDataAt<EthernetControlFrameBase>(frame->getChunkLength(), b(-1));
        if (controlFrame->getOpCode() == ETHERNET_CONTROL_PAUSE) {
            auto pauseFrame = check_and_cast<const EthernetPauseFrame *>(controlFrame.get());
            int pauseUnits = pauseFrame->getPauseTime();
            EV_INFO << "Reception of PAUSE frame " << packet << " successfully completed." << endl;
            delete packet;
            numPauseFramesRcvd++;
            emit(rxPausePkUnitsSignal, pauseUnits);
            processPauseCommand(pauseUnits);
        }
        else if(controlFrame->getOpCode() == ETHERNET_PFC_PAUSE){
            auto pauseFrame = check_and_cast<const EthernetPfcFrame *>(controlFrame.get());
            EV<<"receive PFC PAUSE for priority "<<pauseFrame->getPriority()<<endl;
            receivepause++;
            emit(pfcPausedFrame,pauseFrame);
            delete packet;
        }
        else if(controlFrame->getOpCode() == ETHERNET_PFC_RESUME){
            auto pauseFrame = check_and_cast<const EthernetPfcFrame *>(controlFrame.get());
            EV<<"receive PFC RESUME for priority "<<pauseFrame->getPriority()<<endl;
            receiveresume++;
            emit(pfcResumeFrame,pauseFrame);
            delete packet;
        }
        else {
            EV_INFO << "Received unknown ethernet flow control frame " << EV_FIELD(packet) << ", dropped." << endl;
            delete packet;
        }
    }
    else {
        EV_INFO << "Reception of " << EV_FIELD(packet) << " successfully completed." << endl;
        processReceivedDataFrame(packet, frame);
    }
}

void HiEthernetMac::handleEndIFGPeriod()
{
    ASSERT(nullptr == currentTxFrame);
    if (transmitState != WAIT_IFG_STATE)
        throw cRuntimeError("Not in WAIT_IFG_STATE at the end of IFG period");

    // End of IFG period, okay to transmit
    EV_DETAIL << "IFG elapsed" << endl;
    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || simTime() - intvlStartTime >= maxInterval)
        beginNewInterval(simTime());
    changeTransmissionState(TX_IDLE_STATE);


    if (canDequeuePacket()) {
        Packet *packet = dequeuePacket();
        handleUpperPacket(packet);
    }
}

void HiEthernetMac::handleEndTxPeriod()
{
    // we only get here if transmission has finished successfully
    if (transmitState != TRANSMITTING_STATE)
        throw cRuntimeError("Model error: End of transmission, and incorrect state detected");

    if (nullptr == currentTxFrame)
        throw cRuntimeError("Model error: Frame under transmission cannot be found");

    numFramesSent++;
    numBytesSent += currentTxFrame->getByteLength();
    emit(packetSentToLowerSignal, currentTxFrame); // consider: emit with start time of frame

    emit(transmissionEndedSignal, curTxSignal);
    txFinished();

    const auto& header = currentTxFrame->peekAtFront<EthernetMacHeader>();
    if (header->getTypeOrLength() == ETHERTYPE_FLOW_CONTROL) {
        const auto& controlFrame = currentTxFrame->peekDataAt<EthernetControlFrameBase>(header->getChunkLength(), b(-1));
        if (controlFrame->getOpCode() == ETHERNET_CONTROL_PAUSE) {
            const auto& pauseFrame = CHK(dynamicPtrCast<const EthernetPauseFrame>(controlFrame));
            numPauseFramesSent++;
            emit(txPausePkUnitsSignal, pauseFrame->getPauseTime());
        }
    }

    EV_INFO << "Transmission of " << currentTxFrame << " successfully completed.\n";

    deleteCurrentTxFrame();
    lastTxFinishTime = simTime();

    if (pauseUnitsRequested > 0) {
        // if we received a PAUSE frame recently, go into PAUSE state
        EV_DETAIL << "Going to PAUSE mode for " << pauseUnitsRequested << " time units\n";

        scheduleEndPausePeriod(pauseUnitsRequested);
        pauseUnitsRequested = 0;
    }
    else {
        EV_DETAIL << "Start IFG period\n";
        scheduleEndIFGPeriod();
    }
}

void HiEthernetMac::finish()
{
    EthernetMacBase::finish();

    recordScalar("total packets", numPackets);
    recordScalar("total bits", numBits);

    simtime_t t = simTime();
    simtime_t totalRxChannelIdleTime = t - totalSuccessfulRxTime;
    recordScalar("rx channel idle (%)", 100 * (totalRxChannelIdleTime / t));
    recordScalar("rx channel utilization (%)", 100 * (totalSuccessfulRxTime / t));

    recordScalar("send pause frame", sendpause);
    recordScalar("send resume frame", sendresume);
    recordScalar("receive pause frame", receivepause);
    recordScalar("receive resume frame", receiveresume);
}

void HiEthernetMac::handleEndPausePeriod()
{
    ASSERT(nullptr == currentTxFrame);
    if (transmitState != PAUSE_STATE)
        throw cRuntimeError("End of PAUSE event occurred when not in PAUSE_STATE!");

    EV_DETAIL << "Pause finished, resuming transmissions\n";
    changeTransmissionState(TX_IDLE_STATE);

    if (canDequeuePacket()) {
        Packet *packet = dequeuePacket();
        handleUpperPacket(packet);
    }
}

void HiEthernetMac::processReceivedDataFrame(Packet *packet, const Ptr<const EthernetMacHeader>& frame)
{
    // statistics
    unsigned long curBytes = packet->getByteLength();
    numFramesReceivedOK++;
    numBytesReceivedOK += curBytes;
    emit(rxPkOkSignal, packet);

    const auto macAddressInd = packet->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(frame->getSrc());
    macAddressInd->setDestAddress(frame->getDest());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ethernetMac);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
    if (networkInterface)
        packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());

    numFramesPassedToHL++;
    emit(packetSentToUpperSignal, packet);
    // pass up to upper layer
    EV_INFO << "Sending " << packet << " to upper layer.\n";
    send(packet, upperLayerOutGateId);
}

void HiEthernetMac::processPauseCommand(int pauseUnits)
{
    if (transmitState == TX_IDLE_STATE) {
        EV_DETAIL << "PAUSE frame received, pausing for " << pauseUnitsRequested << " time units\n";
        if (pauseUnits > 0)
            scheduleEndPausePeriod(pauseUnits);
    }
    else if (transmitState == PAUSE_STATE) {
        EV_DETAIL << "PAUSE frame received, pausing for " << pauseUnitsRequested
                  << " more time units from now\n";
        cancelEvent(endPauseTimer);

        // Terminate PAUSE if pauseUnits == 0; Extend PAUSE if pauseUnits > 0
        scheduleEndPausePeriod(pauseUnits);
    }
    else {
        // transmitter busy -- wait until it finishes with current frame (endTx)
        // and then it'll go to PAUSE state
        EV_DETAIL << "PAUSE frame received, storing pause request\n";
        pauseUnitsRequested = pauseUnits;
    }
}

void HiEthernetMac::scheduleEndIFGPeriod()
{
    ASSERT(nullptr == currentTxFrame);
    changeTransmissionState(WAIT_IFG_STATE);
    simtime_t endIFGTime = simTime() + (b(INTERFRAME_GAP_BITS).get() / curEtherDescr->txrate);
    scheduleAt(endIFGTime, endIfgTimer);
}

void HiEthernetMac::scheduleEndPausePeriod(int pauseUnits)
{
    ASSERT(nullptr == currentTxFrame);
    // length is interpreted as 512-bit-time units
    simtime_t pausePeriod = ((pauseUnits * PAUSE_UNIT_BITS) / curEtherDescr->txrate);
    scheduleAfter(pausePeriod, endPauseTimer);
    changeTransmissionState(PAUSE_STATE);
}

void HiEthernetMac::beginSendFrames()
{
    if (currentTxFrame) {
        // Other frames are queued, transmit next frame
        EV_DETAIL << "Transmit next frame in output queue\n";
        startFrameTransmission();
    }
    else {
        // No more frames set transmitter to idle
        changeTransmissionState(TX_IDLE_STATE);
        EV_DETAIL << "No more frames to send, transmitter set to idle\n";
    }
}

void HiEthernetMac::handleCanPullPacketChanged(cGate *gate)
{
    EV_INFO<<"HiEthernetMac::handleCanPullPacketChanged()"<<endl;
    Enter_Method("handleCanPullPacketChanged");
    if (currentTxFrame == nullptr && transmitState == TX_IDLE_STATE && canDequeuePacket() ){//&& only) {
        Packet *packet = dequeuePacket();
        handleUpperPacket(packet);
    }
}

void HiEthernetMac::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(signalID==REDPFCQueue::pfcPausedSignal||signalID==REDPFCQueue::pfcResumeSignal
            ){
        EV<<"HiEthernetMac::receiveSignal(), receive pfc signal"<<endl;
        processpfccommand(obj);
    }
}

void HiEthernetMac::processpfccommand(cObject *obj){
    Enter_Method("processpfccommand");
    auto tag=check_and_cast<Packet *>(obj)->getTag<HiTag>();
    if(networkInterface->getInterfaceId()==tag->getInterfaceId()){
        EV<<"pfc command to "<<networkInterface->getInterfaceName()<<endl;
        while(currentTxFrame==nullptr&& transmitState == TX_IDLE_STATE){
            if(tag->getOp()==ETHERNET_PFC_PAUSE){
                EV<<"send pause frame"<<endl;
                sendpause++;
                auto packet = new Packet("Pause");
                const auto& frame = makeShared<EthernetPfcFrame>();
                const auto& hdr = makeShared<EthernetMacHeader>();
                frame->setOpCode(ETHERNET_PFC_PAUSE);
                frame->setPriority(tag->getPriority());
                packet->insertAtFront(frame);
                hdr->setTypeOrLength(ETHERTYPE_FLOW_CONTROL);
                hdr->setDest(MacAddress::MULTICAST_PAUSE_ADDRESS);
                hdr->setSrc(getMacAddress());
                packet->insertAtFront(hdr);
                const auto& ethernetFcs = makeShared<EthernetFcs>();
                ethernetFcs->setFcsMode(fcsMode);
                packet->insertAtBack(ethernetFcs);
                addPaddingAndSetFcs(packet, MIN_ETHERNET_FRAME_BYTES);
                currentTxFrame = packet;
                startFrameTransmission();
            }else if(tag->getOp()==ETHERNET_PFC_RESUME){
                EV<<"send resume frame"<<endl;
                sendresume++;
                auto packet = new Packet("Resume");
                const auto& frame = makeShared<EthernetPfcFrame>();
                const auto& hdr = makeShared<EthernetMacHeader>();
                frame->setOpCode(ETHERNET_PFC_RESUME);
                frame->setPriority(tag->getPriority());
                packet->insertAtFront(frame);
                hdr->setTypeOrLength(ETHERTYPE_FLOW_CONTROL);
                hdr->setDest(MacAddress::MULTICAST_PAUSE_ADDRESS);
                hdr->setSrc(getMacAddress());
                packet->insertAtFront(hdr);
                const auto& ethernetFcs = makeShared<EthernetFcs>();
                ethernetFcs->setFcsMode(fcsMode);
                packet->insertAtBack(ethernetFcs);
                addPaddingAndSetFcs(packet, MIN_ETHERNET_FRAME_BYTES);
                currentTxFrame = packet;
                startFrameTransmission();
            }
        }
    }
}

} // namespace inet

