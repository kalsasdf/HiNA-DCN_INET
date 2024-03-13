/*
 * BCNMac.cc
 *
 *  Created on: 2023年10月23日
 *      Author: ergeng2001
 */



#include "BCNMac.h"


namespace inet {

// TODO refactor using a statemachine that is present in a single function
// TODO this helps understanding what interactions are there and how they affect the state

Define_Module(BCNMac);

simsignal_t BCNMac::bfcPausedFrame =
        cComponent::registerSignal("bfcpauseframe");
simsignal_t BCNMac::bfcResumeFrame =
        cComponent::registerSignal("bfcresumeframe");

BCNMac::BCNMac()
{
}

void BCNMac::initialize(int stage)
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
        WATCH(senddeceleration);
        WATCH(sendspeedup);

        bitpersecVector.setName("thruput (bps)");
        pkpersecVector.setName("packet/sec");
        cModule *radioModule = getParentModule()->getParentModule();EV<<"parentmodule = "<<radioModule<<endl;
        //subscribe mac->eth->router
        radioModule->subscribe(BCNqueue::bfcPausedSignal,this);
        radioModule->subscribe(BCNqueue::bfcResumeSignal,this);

        cModule *radioModule_eth = getParentModule();
        EV<<"parentmodule_eth = "<<radioModule_eth<<endl;
        radioModule_eth->subscribe(BCNqueue::bfcDecelerationSignal,this);
        radioModule_eth->subscribe(BCNqueue::bfcSpeedupSignal,this);

        TIMELY=par("TIMELY");
        HPCC=par("HPCC");

        if (!par("duplexMode"))
            throw cRuntimeError("Half duplex operation is not supported by BCNMac, use the EthernetCsmaMac module for that! (Please enable csmacdSupport on EthernetInterface)");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        beginSendFrames(); // FIXME choose an another stage for it
    }
}

void BCNMac::initializeStatistics()
{
    EthernetMacBase::initializeStatistics();

    // initialize statistics
    totalSuccessfulRxTime = 0.0;
}

void BCNMac::initializeFlags()
{
    EthernetMacBase::initializeFlags();

    duplexMode = true;
    physInGate->setDeliverImmediately(false);
}

void BCNMac::handleMessageWhenUp(cMessage *msg)
{
    if (channelsDiffer)
        readChannelParameters(true);

    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else if (msg->getArrivalGateId() == upperLayerInGateId){
//        Packet pck = check_and_cast<Packet *>(msg)
        EV<<"prepare handle upPcket "<< msg <<" ,simtime ="<<simTime()<< endl;

        handleUpperPacket(check_and_cast<Packet *>(msg));
    }

    else if (msg->getArrivalGate() == physInGate)
        processMsgFromNetwork(check_and_cast<EthernetSignalBase *>(msg));
    else
        throw cRuntimeError("Message received from unknown gate!");
    processAtHandleMessageFinished();
}

void BCNMac::handleSelfMessage(cMessage *msg)
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

void BCNMac::startFrameTransmission()
{
    ASSERT(currentTxFrame);
    EV_DETAIL << "Transmitting a copy of frame " << currentTxFrame << endl;

    Packet *frame = currentTxFrame->dup(); // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()
    const auto& hdr = frame->peekAtFront<EthernetMacHeader>(); // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()
    ASSERT(hdr);
    ASSERT(!hdr->getSrc().isUnspecified());

    // add preamble and SFD (Starting Frame Delimiter), then send out
    encapsulate(frame);
    updateStats(simTime(), frame);//for rate-meter

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
    signal->encapsulate(frame);//没起作用

    ASSERT(curTxSignal == nullptr);
    curTxSignal = signal->dup();
    emit(transmissionStartedSignal, signal);
    EV<<"send frame from mac to phy , "<<frame << ", simTime = "<<simTime()<<endl;
    send(signal, physOutGate);

    scheduleAt(transmissionChannel->getTransmissionFinishTime(), endTxTimer);
    changeTransmissionState(TRANSMITTING_STATE);
}

void BCNMac::updateStats(simtime_t now, Packet *pck)
{
    numPackets++;
    int bits=pck->getBitLength();
    numBits += bits;

    int priority;

    if(priority>11||priority<0)
        priority = 10;

    intvlNumPackets++;
    intvlNumBits += bits;
    EV<<"intvlNumbits = "<<intvlNumBits<<", intvlNumPackets = "<<intvlNumPackets<<endl;
    intvlPriBits[priority] += bits;

    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || now - intvlStartTime >= maxInterval)
        beginNewInterval(now);
}

void BCNMac::beginNewInterval(simtime_t now)
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

void BCNMac::handleUpperPacket(Packet *packet)
{
    EV_INFO << "Received " << packet << " from upper layer. simTime = "<<simTime() << endl;

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
        throw cRuntimeError("BCNMac not in TX_IDLE_STATE when packet arrived from upper layer");
    if (currentTxFrame != nullptr)
        throw cRuntimeError("BCNMac already has a transmit packet when packet arrived from upper layer");
    addPaddingAndSetFcs(packet, MIN_ETHERNET_FRAME_BYTES);


    currentTxFrame = packet;
    startFrameTransmission();
}

void BCNMac::processMsgFromNetwork(EthernetSignalBase *signal)
{
    EV_INFO <<"BCNMac::processMsgFromNetwork(), "<< signal << " received." << endl;

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
    EV<<"packet = "<<packet <<" , simTime = "<< simTime()<<endl;
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
            emit(rxPausePkUnitsSignal, pauseUnits);//pause time not use;
            processPauseCommand(pauseUnits);
        }
        else if(controlFrame->getOpCode() == ETHERNET_BFC_PAUSE){
            auto pauseFrame = check_and_cast<const EthernetBfcFrame *>(controlFrame.get());
            EV<<"In BFCMAC receive BFC PAUSE for QueueNum "<<pauseFrame->getQueueID()<<endl;
            receivepause++;
            emit(bfcPausedFrame,pauseFrame);
            delete packet;
        }
        else if(controlFrame->getOpCode() == ETHERNET_BFC_RESUME){
            auto pauseFrame = check_and_cast<const EthernetBfcFrame *>(controlFrame.get());
            EV<<"In  BFCMAC receive BFC RESUME for QueueNum "<<pauseFrame->getQueueID()<<endl;
            receiveresume++;
            emit(bfcResumeFrame,pauseFrame);
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

void BCNMac::handleEndIFGPeriod()
{
    ASSERT(nullptr == currentTxFrame);
    if (transmitState != WAIT_IFG_STATE)
        throw cRuntimeError("Not in WAIT_IFG_STATE at the end of IFG period");

    // End of IFG period, okay to transmit
    EV_DETAIL << "IFG elapsed" << endl;
    changeTransmissionState(TX_IDLE_STATE);


    if (canDequeuePacket()) {
        Packet *packet = dequeuePacket();
        handleUpperPacket(packet);
    }
}

void BCNMac::handleEndTxPeriod()
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

void BCNMac::finish()
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

    recordScalar("send deceleration bfccnp",senddeceleration);
    recordScalar("send speedup bfccnp",sendspeedup);
}

void BCNMac::handleEndPausePeriod()
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

void BCNMac::processReceivedDataFrame(Packet *packet, const Ptr<const EthernetMacHeader>& frame)
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

void BCNMac::processPauseCommand(int pauseUnits)
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

void BCNMac::scheduleEndIFGPeriod()
{
    ASSERT(nullptr == currentTxFrame);
    changeTransmissionState(WAIT_IFG_STATE);
    simtime_t endIFGTime = simTime() + (b(INTERFRAME_GAP_BITS).get() / curEtherDescr->txrate);
    scheduleAt(endIFGTime, endIfgTimer);
}

void BCNMac::scheduleEndPausePeriod(int pauseUnits)
{
    ASSERT(nullptr == currentTxFrame);
    // length is interpreted as 512-bit-time units
    simtime_t pausePeriod = ((pauseUnits * PAUSE_UNIT_BITS) / curEtherDescr->txrate);
    scheduleAfter(pausePeriod, endPauseTimer);
    changeTransmissionState(PAUSE_STATE);
}

void BCNMac::beginSendFrames()
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

void BCNMac::handleCanPullPacketChanged(cGate *gate)
{
    EV_INFO<<"BCNMac::handleCanPullPacketChanged()"<<endl;
    Enter_Method("handleCanPullPacketChanged");
    EV<<"mac handle can pull packet, simTime = "<<simTime()<<endl;
    if (currentTxFrame == nullptr && transmitState == TX_IDLE_STATE && canDequeuePacket() ){//&& only) {
        EV<<"mac start dequeuePacket, simTime = "<<simTime()<<endl;
        Packet *packet = dequeuePacket();//调用Hiqueue的pull函数
        handleUpperPacket(packet);
    }
}

void BCNMac::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(signalID==BCNqueue::bfcPausedSignal||signalID==BCNqueue::bfcResumeSignal){
        cModule *radioModule = getParentModule()->getParentModule();
        processbfccommand(obj);

    }
    if(signalID==BCNqueue::bfcDecelerationSignal||signalID==BCNqueue::bfcSpeedupSignal){
        cModule *radioModule = getParentModule()->getParentModule();
        processsendbfccnp(obj);
    }
}

void BCNMac::processbfccommand(cObject *obj){
    Enter_Method("processbfccommand");
    auto tag=check_and_cast<Packet *>(obj)->getTag<HiTag>();

    if(networkInterface->getInterfaceId()==tag->getInterfaceId()){//发送给指定端口
        EV<<"-----------------BCNMac process sent BCN command-------------------"<<endl;
        EV<<"BCN command to( This networkInter is ) "<<networkInterface->getInterfaceId()<<endl;
        EV<<"This tag->getInterfaceId() is "<<tag->getInterfaceId()<<endl;

        while(currentTxFrame==nullptr&& transmitState == TX_IDLE_STATE){
            if(tag->getOp()==ETHERNET_BFC_PAUSE){
                EV<<"send pause frame"<<endl;
                sendpause++;
                EV<<"In mac sendpaused = "<< sendpause <<endl;
                auto packet = new Packet("Pause");
                const auto& frame = makeShared<EthernetBfcFrame>();
                const auto& hdr = makeShared<EthernetMacHeader>();
                frame->setOpCode(ETHERNET_BFC_PAUSE);
                frame->setQueueID(tag->getQueueID());
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
                EV<<"----------this BCNMac had sent Pause packet-------------------"<<endl;
            }else if(tag->getOp()==ETHERNET_BFC_RESUME){
                EV<<"send resume frame"<<endl;
                sendresume++;
                EV<<"In mac sendresume = "<< sendresume <<endl;
                auto packet = new Packet("Resume");
                const auto& frame = makeShared<EthernetBfcFrame>();
                const auto& hdr = makeShared<EthernetMacHeader>();
                frame->setOpCode(ETHERNET_BFC_RESUME);
                frame->setQueueID(tag->getQueueID());
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
                EV<<"----------this BCNMac had sent Resume packet-------------------"<<endl;
            }
        }
    }

}

void BCNMac::processsendbfccnp(cObject *obj){
    Enter_Method("processcnpsend");
        auto tag=check_and_cast<Packet *>(obj)->getTag<HiTag>();
        if(networkInterface->getInterfaceId()==tag->getLocal_interfaceId()){//发送给本eth端口
            EV<<"-----------------Local BCNMac process send routerCNP-------------------"<<endl;
            EV<<"This tag->getInterfaceId() is "<<tag->getInterfaceId()<<endl;

                if(tag->getOp()==ETHERNET_BFC_DECELERATION){
                    EV<<"send deceleration frame"<<endl;
                    senddeceleration++;
                    EV<<"In mac had sent bfc deceleration cnp = "<< senddeceleration <<", flowid = "<<tag->getFlowId()<<endl;
                    std::ostringstream str;
                    str << "routerCNP_del_" << tag->getFlowId();
                    Packet* bfccnp = new Packet(str.str().c_str());
                    const auto& payload = makeShared<ByteCountChunk>(B(1));
                    auto bfccnpTag = payload->addTag<HiTag>();
                    bfccnpTag->setFlowId(tag->getFlowId());
                    bfccnpTag->setIsDeceleration(true);
                    bfccnpTag->setBfccnpECN(1);//当前是拥塞交换机
                    bfccnp->insertAtBack(payload);

                    auto addressReq = bfccnp->addTagIfAbsent<L3AddressReq>();
                    addressReq->setSrcAddress(networkInterface->getIpv4Address());
                   // addressReq->setSrcAddress(nullptr);
                    addressReq->setDestAddress(tag->getSrcAddr());
                    bfccnp->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
                    bfccnp->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

                    send(bfccnp, upperLayerOutGateId);

                }else if(tag->getOp()==ETHERNET_BFC_SPEEDUP){
                    EV<<"send speedup frame"<<endl;
                    sendspeedup++;
                    EV<<"In mac had sent bfc speedup cnp = "<< sendspeedup <<", flowid = "<<tag->getFlowId()<<endl;
                    std::ostringstream str;
                    str << "routerCNP_spe_" << tag->getFlowId();
                    Packet* bfccnp = new Packet(str.str().c_str());
                    const auto& payload = makeShared<ByteCountChunk>(B(1));
                    auto bfccnpTag = payload->addTag<HiTag>();
                    bfccnpTag->setFlowId(tag->getFlowId());
                    bfccnpTag->setIsSpeedup(true);
                    bfccnpTag->setBfccnpECN(1);//当前是拥塞交换机
                    bfccnp->insertAtBack(payload);


                    auto addressReq = bfccnp->addTagIfAbsent<L3AddressReq>();
                    addressReq->setSrcAddress(networkInterface->getIpv4Address());
                   // addressReq->setSrcAddress(nullptr);
                    addressReq->setDestAddress(tag->getSrcAddr());
                    bfccnp->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
                    bfccnp->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

                    send(bfccnp, upperLayerOutGateId);
                }

        }

}

} // namespace inet






