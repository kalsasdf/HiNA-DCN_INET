/*
 * HiEthernetEncaplation.cc
 *
 *  Created on: 2023Äê10ÔÂ20ÈÕ
 *      Author: ergeng2001
 */
//
// Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "HiEthernetEncapsulation.h"

namespace inet {

Define_Module(HiEthernetEncapsulation);

simsignal_t HiEthernetEncapsulation::encapPkSignal = registerSignal("encapPk");
simsignal_t HiEthernetEncapsulation::decapPkSignal = registerSignal("decapPk");
simsignal_t HiEthernetEncapsulation::pauseSentSignal = registerSignal("pauseSent");

std::ostream& operator<<(std::ostream& o, const HiEthernetEncapsulation::Socket& t)
{
    o << "(id:" << t.socketId
      << ",interfaceId:" << t.interfaceId
      << ",local:" << t.localAddress
      << ",remote:" << t.remoteAddress
      << ",protocol" << (t.protocol ? t.protocol->getName() : "<null>")
      << ",steal:" << (t.steal ? "on" : "off")
      << ")";
    return o;
}

HiEthernetEncapsulation::~HiEthernetEncapsulation()
{
    for (auto it : socketIdToSocketMap)
        delete it.second;
}

bool HiEthernetEncapsulation::Socket::matches(Packet *packet, int ifaceId, const Ptr<const EthernetMacHeader>& ethernetMacHeader)
{
    if (interfaceId != -1 && interfaceId != ifaceId)
        return false;
    if (!remoteAddress.isUnspecified() && !ethernetMacHeader->getSrc().isBroadcast() && ethernetMacHeader->getSrc() != remoteAddress)
        return false;
    if (!localAddress.isUnspecified() && !ethernetMacHeader->getDest().isBroadcast() && ethernetMacHeader->getDest() != localAddress)
        return false;
    if (protocol != nullptr && packet->getTag<PacketProtocolTag>()->getProtocol() != protocol)
        return false;
    return true;
}

void HiEthernetEncapsulation::initialize(int stage)
{
    OperationalBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        fcsMode = parseFcsMode(par("fcsMode"));
        seqNum = 0;
        WATCH(seqNum);
        totalFromHigherLayer = totalFromMAC = totalPauseSent = 0;
        networkInterface = findContainingNicModule(this); // TODO or getContainingNicModule() ? or use a MacForwardingTable?

        WATCH_PTRSET(upperProtocols);
        WATCH_PTRMAP(socketIdToSocketMap);
        WATCH(totalFromHigherLayer);
        WATCH(totalFromMAC);
        WATCH(totalPauseSent);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        if (par("registerProtocol").boolValue()) { // FIXME //KUDGE should redesign place of HiEthernetEncapsulation and LLC modules
            // register service and protocol
            registerService(Protocol::ethernetMac, gate("upperLayerIn"), gate("upperLayerOut"));
            registerProtocol(Protocol::ethernetMac, gate("lowerLayerOut"), gate("lowerLayerIn"));
        }
    }
}

void HiEthernetEncapsulation::handleMessageWhenUp(cMessage *msg)
{
    if (msg->arrivedOn("upperLayerIn")) {
        // from upper layer
        EV_INFO << "Received " << msg << " from upper layer." << endl;
        if (msg->isPacket())
            processPacketFromHigherLayer(check_and_cast<Packet *>(msg));
        else
            processCommandFromHigherLayer(check_and_cast<Request *>(msg));
    }
    else if (msg->arrivedOn("lowerLayerIn")) {

        EV_INFO << "Received " << msg << " from lower layer." << endl;
        Packet* pck = check_and_cast<Packet*>(msg);

        if(std::string(check_and_cast<Packet*>(msg)->getFullName()).find("routerCNP") != std::string::npos){//for BFC packet
            EV<<"Ethernet receive routerCNP."<<endl;
            b offset(0);
            auto ethHeader2  = pck->peekDataAt<EthernetMacHeader>(offset);
            EV<<"ethHeader2 = "<<ethHeader2<<endl;
            if(ethHeader2 == nullptr){
                    EV<<"the first get msg."<<endl;
                    EV<<"send to upper layer."<<endl;
                    send(pck, "upperLayerOut");

            }else{
                processPacketFromMac(check_and_cast<Packet *>(msg));
                }
        }else{
            processPacketFromMac(check_and_cast<Packet *>(msg));
            }
    }
    else
        throw cRuntimeError("Unknown gate");
}

void HiEthernetEncapsulation::processCommandFromHigherLayer(Request *msg)
{
    msg->removeTagIfPresent<DispatchProtocolReq>();
    auto ctrl = msg->getControlInfo();
    if (dynamic_cast<Ieee802PauseCommand *>(ctrl) != nullptr)
        handleSendPause(msg);
    else if (auto bindCommand = dynamic_cast<EthernetBindCommand *>(ctrl)) {
        int socketId = check_and_cast<Request *>(msg)->getTag<SocketReq>()->getSocketId();
        Socket *socket = new Socket(socketId);
        socket->interfaceId = msg->getTag<InterfaceReq>()->getInterfaceId();
        socket->localAddress = bindCommand->getLocalAddress();
        socket->remoteAddress = bindCommand->getRemoteAddress();
        socket->protocol = bindCommand->getProtocol();
        socket->steal = bindCommand->getSteal();
        socketIdToSocketMap[socketId] = socket;
        delete msg;
    }
    else if (dynamic_cast<SocketCloseCommand *>(ctrl) != nullptr) {
        int socketId = check_and_cast<Request *>(msg)->getTag<SocketReq>()->getSocketId();
        auto it = socketIdToSocketMap.find(socketId);
        if (it != socketIdToSocketMap.end()) {
            delete it->second;
            socketIdToSocketMap.erase(it);
            auto indication = new Indication("closed", SOCKET_I_CLOSED);
            auto ctrl = new SocketClosedIndication();
            indication->setControlInfo(ctrl);
            indication->addTag<SocketInd>()->setSocketId(socketId);
            send(indication, "transportOut");
            delete msg;
        }
    }
    else if (dynamic_cast<SocketDestroyCommand *>(ctrl) != nullptr) {
        int socketId = check_and_cast<Request *>(msg)->getTag<SocketReq>()->getSocketId();
        auto it = socketIdToSocketMap.find(socketId);
        if (it != socketIdToSocketMap.end()) {
            delete it->second;
            socketIdToSocketMap.erase(it);
            delete msg;
        }
    }
    else
        throw cRuntimeError("Unknown command: '%s' with %s", msg->getName(), ctrl->getClassName());
}

void HiEthernetEncapsulation::refreshDisplay() const
{
    OperationalBase::refreshDisplay();
    char buf[80];
    sprintf(buf, "passed up: %ld\nsent: %ld", totalFromMAC, totalFromHigherLayer);
    getDisplayString().setTagArg("t", 0, buf);
}

void HiEthernetEncapsulation::processPacketFromHigherLayer(Packet *packet)
{
    if (packet->getDataLength() > MAX_ETHERNET_DATA_BYTES)
        throw cRuntimeError("packet length from higher layer (%s) exceeds maximum Ethernet payload length (%s)", packet->getDataLength().str().c_str(), MAX_ETHERNET_DATA_BYTES.str().c_str());

    totalFromHigherLayer++;
    emit(encapPkSignal, packet);

    //for BFC/BCN/AFC packet£¬insert BFCHeader. Ethernet -> BFCqueue -> Classifier
    if ((std::string(packet->getFullName()).find("BCN") != std::string::npos)||(std::string(packet->getFullName()).find("BFC") != std::string::npos)||(std::string(packet->getFullName()).find("AFC") != std::string::npos))
    {
        auto content = makeShared<BFCHeader>();
        content->enableImplicitChunkSerialization = true;
        content ->setQueueID(0);
        cModule *radioModule = getParentModule();

        //1¡¢check sender : set upstreamQid = 0, set queueid = 0;
        if (std::string(radioModule ->getFullName()).find("s")!= std::string::npos){//for sender
              EV<<"This's sender, insert BFCHeader's upstreamQid = 0, queueID = 0."<<endl;
              content->setUpstreamQueueID(0);
              content->setQueueID(0);

        //2¡¢check switch : find map<flowid, upstreamQid>
        }else{//for switch£¬set UpstreamQueueid
            int upstreamQueueid;

            //2.1 find packet flowid
            auto& HiPacket = packet -> peekAtFront<ByteCountChunk>();
            int flowID;
            for(auto& region:packet->peekData()->getAllTags<HiTag>()){
                  flowID = region.getTag()->getFlowId();
            }

            //2.2 find map<flowid, upstreamQid>
            flowQueuePair itrf;
            itrf = flow_upstreamQueue.find(flowID);
            if(itrf == flow_upstreamQueue.end())//Error! not find flowid.
                EV<<"Error! map<flowid,upstreamQid> not find flowID = "<< flowID <<endl;
            else{//find map<flowid,upstreamQid>
                EV<<"map<flowid,upstreamQid> find flowID = "<< flowID <<endl;
                upstreamQueueid = itrf->second;

                //2.3 check the last packet
                bool last = false;
                for(auto& region: packet->peekData()->getAllTags<HiTag>()){
                     last = region.getTag()->isLastPck();
                }
                if(last){//delete <flowid, upstreamQid>
                    flow_upstreamQueue.erase(itrf);
                }
            }

            //2.4 for switch : set queueid = 0, set upstreamQid = upstreamQueueid;
            content->setQueueID(0);
            content->setUpstreamQueueID(upstreamQueueid);
            EV<<"This switch, insert BFCHeader's upstreamQid = "<< upstreamQueueid<<endl;
        }

        packet->insertAtFront(content);
        EV_INFO<< packet <<" had insert BFCHeader."<<endl;
     }//inser BFCHeader

    // Creates MAC header information and encapsulates received higher layer data
    // with this information and transmits resultant frame to lower layer

    // create Ethernet frame, fill it in from Ieee802Ctrl and encapsulate msg in it
    EV_DETAIL << "Encapsulating higher layer packet `" << packet << "' for MAC\n";

    int typeOrLength = -1;
    const auto& protocolTag = packet->addTagIfAbsent<PacketProtocolTag>();
    const Protocol *protocol = protocolTag->getProtocol();
    if (protocol && *protocol != Protocol::ieee8022llc)
        typeOrLength = ProtocolGroup::ethertype.getProtocolNumber(protocol);
    else
        typeOrLength = packet->getByteLength();

    const auto& ethHeader = makeShared<EthernetMacHeader>();
    auto macAddressReq = packet->getTag<MacAddressReq>();
    auto srcAddr = macAddressReq->getSrcAddress();
    if (srcAddr.isUnspecified() && networkInterface != nullptr)
        srcAddr = networkInterface->getMacAddress();
    ethHeader->setSrc(srcAddr);
    ethHeader->setDest(macAddressReq->getDestAddress());
    ethHeader->setTypeOrLength(typeOrLength);
    packet->insertAtFront(ethHeader);
    EV_DETAIL << "Encapsulating EthernetMacHeader `" << packet << "' for MAC\n";
    const auto& ethernetFcs = makeShared<EthernetFcs>();
    ethernetFcs->setFcsMode(fcsMode);
    packet->insertAtBack(ethernetFcs);
    EV_DETAIL << "Encapsulating ethernetFcs `" << packet << "' for MAC\n";
    protocolTag->setProtocol(&Protocol::ethernetMac);
    packet->removeTagIfPresent<DispatchProtocolReq>();
    EV_INFO << "Sending " << packet << " to lower layer.\n";
    EV<<"BCN packet from ethernet send to mac. packet = "<<packet <<", simTime() = "<< simTime()<<endl;
    send(packet, "lowerLayerOut");
}

void HiEthernetEncapsulation::processPacketFromMac(Packet *packet)
{
    const Protocol *payloadProtocol = nullptr;

    //1¡¢find packet come eth information
    int iface = packet->getTag<InterfaceInd>()->getInterfaceId();
    auto ethHeader = packet->popAtFront<EthernetMacHeader>();
//    b offset(0);
//    auto ethHeader = packet->removeDataAt<EthernetMacHeader>(offset);
    EV_INFO<<"In ethernet, "<<packet <<" from mac, popAtFront<EthernetMacHeader>() !"<<endl;

    packet->popAtBack<EthernetFcs>(ETHER_FCS_BYTES);
    EV_INFO<<"In ethernet, "<<packet <<" from mac, popAtBack<EthernetFcs>(ETHER_FCS_BYTES) !"<<endl;

    //2¡¢add MacAddressInd to packet
    auto macAddressInd = packet->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(ethHeader->getSrc());
    macAddressInd->setDestAddress(ethHeader->getDest());

    //3¡¢for BCN\BFC\AFC packet, delete bfcHeader, save upstreamQid
   if ((std::string(packet->getFullName()).find("BCN") != std::string::npos)||(std::string(packet->getFullName()).find("BFC") != std::string::npos)||(std::string(packet->getFullName()).find("AFC") != std::string::npos))
   {
       //3.1 remove BFCHeader (for switch and receiver)
       auto bfcHeader = packet->removeAtFront<BFCHeader>();
       int upstreamQid = bfcHeader->getUpstreamQueueID();
       EV_INFO<<"In Ethernet, "<<packet <<" from mac, popAtFront<BFCHeader>() !"<<endl;

       //3.2 get packet's flowid
       auto& HiPacket = packet -> peekAtFront<ByteCountChunk>();
       int flowID;
       for(auto& region:packet->peekData()->getAllTags<HiTag>()){
            flowID = region.getTag()->getFlowId();
       }
       EV<<packet << " flowid = "<< flowID <<endl;

       //3.3 find map<flowid,upstreamQid>

       //3.3.1 OK! find map<flowid,upstreamQid>
       if(flow_upstreamQueue.find(flowID)!=flow_upstreamQueue.end()){//flow in map<flowid,upstreamQid>
           EV<<"Packet'flowid in map <flowid, upstreamQueueid>"<<endl;

       //3.3.2 No! not find map<flowid,upstreamQid>
       }else{//flow not in map<flowid,upstreamQid>
           EV<<"Packet'flowid not in map <flowid, upstreamQueueid> "<<endl;

           //3.3.3 insert map<flowid,upstreamQid>
           if(flow_upstreamQueue.empty()){
               flow_upstreamQueue.insert(std::pair<uint64_t,uint32_t>(flowID,upstreamQid));
               flowQueuePair fptr;
               fptr= flow_upstreamQueue.begin();
           }else{
               flow_upstreamQueue.insert(std::pair<uint64_t,uint32_t>(flowID,upstreamQid));
           }
       }
    }// for "BFC"¡¢¡°BCN¡± ¡¢¡°AFC¡± packet.

    // remove Padding if possible
    if (isIeee8023Header(*ethHeader)) {
        b payloadLength = B(ethHeader->getTypeOrLength());
        if (packet->getDataLength() < payloadLength)
            throw cRuntimeError("incorrect payload length in ethernet frame");      // TODO alternative: drop packet
        packet->setBackOffset(packet->getFrontOffset() + payloadLength);
        payloadProtocol = &Protocol::ieee8022llc;
    }
    else if (isEth2Header(*ethHeader)) {
        payloadProtocol = ProtocolGroup::ethertype.findProtocol(ethHeader->getTypeOrLength());
    }
    else
        throw cRuntimeError("Unknown ethernet header");

    if (payloadProtocol != nullptr) {
        packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
        packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    }
    else {
        packet->removeTagIfPresent<PacketProtocolTag>();
        packet->removeTagIfPresent<DispatchProtocolReq>();
    }
//    packet->addTag<PacketProtocolTag>()->setProtocol(&Protocol::ethernetBFC);

    bool steal = false;
    for (auto it : socketIdToSocketMap) {
        auto socket = it.second;
        if (socket->matches(packet, iface, ethHeader)) {
            auto packetCopy = packet->dup();
            packetCopy->setKind(SOCKET_I_DATA);
            packetCopy->addTagIfAbsent<SocketInd>()->setSocketId(it.first);
            EV_INFO << "Sending " << packetCopy << " to socket " << it.first << ".\n";
            send(packetCopy, "upperLayerOut");
            steal |= socket->steal;
        }
    }
    if (steal)
        delete packet;
    else if (anyUpperProtocols || (payloadProtocol != nullptr && contains(upperProtocols, payloadProtocol))) {
        EV_DETAIL << "Decapsulating frame " << packet << " , passing up contained packet `"
                  << packet->getName() << "' to higher layer\n";

        totalFromMAC++;
        emit(decapPkSignal, packet);

        // pass up to higher layers.
        EV_INFO << "Sending " << packet << " to upper layer.\n";
        send(packet, "upperLayerOut");
    }
    else {
        EV_WARN << "Unknown protocol, dropping packet\n";
        PacketDropDetails details;
        details.setReason(NO_PROTOCOL_FOUND);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

void HiEthernetEncapsulation::handleSendPause(cMessage *msg)
{
    Ieee802PauseCommand *etherctrl = dynamic_cast<Ieee802PauseCommand *>(msg->getControlInfo());
    if (!etherctrl)
        throw cRuntimeError("PAUSE command `%s' from higher layer received without Ieee802PauseCommand controlinfo", msg->getName());
    MacAddress dest = etherctrl->getDestinationAddress();
    int pauseUnits = etherctrl->getPauseUnits();
    delete msg;

    EV_DETAIL << "Creating and sending PAUSE frame, with duration = " << pauseUnits << " units\n";

    // create Ethernet frame
    char framename[40];
    sprintf(framename, "pause-%d-%d", getId(), seqNum++);
    auto packet = new Packet(framename);
    const auto& frame = makeShared<EthernetPauseFrame>();
    const auto& hdr = makeShared<EthernetMacHeader>();
    frame->setPauseTime(pauseUnits);
    if (dest.isUnspecified())
        dest = MacAddress::MULTICAST_PAUSE_ADDRESS;
    hdr->setDest(dest);
    packet->insertAtFront(frame);
    hdr->setTypeOrLength(ETHERTYPE_FLOW_CONTROL);
    packet->insertAtFront(hdr);
    const auto& ethernetFcs = makeShared<EthernetFcs>();
    ethernetFcs->setFcsMode(fcsMode);
    packet->insertAtBack(ethernetFcs);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);

    EV_INFO << "Sending " << frame << " to lower layer.\n";
    send(packet, "lowerLayerOut");

    emit(pauseSentSignal, pauseUnits);
    totalPauseSent++;
}

void HiEthernetEncapsulation::clearSockets()
{
    for (auto& elem : socketIdToSocketMap) {
        delete elem.second;
        elem.second = nullptr;
    }
    socketIdToSocketMap.clear();
}

void HiEthernetEncapsulation::handleStartOperation(LifecycleOperation *operation)
{
    clearSockets();
}

void HiEthernetEncapsulation::handleStopOperation(LifecycleOperation *operation)
{
    clearSockets();
}

void HiEthernetEncapsulation::handleCrashOperation(LifecycleOperation *operation)
{
    clearSockets();
}

void HiEthernetEncapsulation::handleRegisterService(const Protocol& protocol, cGate *gate, ServicePrimitive servicePrimitive)
{
    Enter_Method("handleRegisterService");
}

void HiEthernetEncapsulation::handleRegisterProtocol(const Protocol& protocol, cGate *gate, ServicePrimitive servicePrimitive)
{
    Enter_Method("handleRegisterProtocol");
    // KLUDGE this should be here: if (!strcmp("upperLayerOut", gate->getBaseName()))
    // but then the register protocol calls are lost, because they can't go through the traffic conditioner
    upperProtocols.insert(&protocol);
}

void HiEthernetEncapsulation::handleRegisterAnyProtocol(cGate *gate, ServicePrimitive servicePrimitive)
{
    Enter_Method("handleRegisterAnyProtocol");
    if (!strcmp("upperLayerOut", gate->getBaseName()))
        anyUpperProtocols = true;
}

} // namespace inet





