//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "HiUdpApp.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"

namespace inet {

Define_Module(HiUdpApp);

uint32_t HiUdpApp::flowid=0;

HiUdpApp::~HiUdpApp()
{
    cancelAndDelete(selfMsg);
}

void HiUdpApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {EV<<"HiUdpApp::initialize stage = "<<stage<<endl;
        FCT_Vector.setName("FCT");
        goodputVector.setName("goodput (bps)");

        WATCH(numSent);
        WATCH(numReceived);

        flow_completion_time.clear();

        workLoad = par("workLoad");
        trafficMode = par("trafficMode");
        linkSpeed = par("linkSpeed");
        localPort = par("localPort");
        connectPort = par("connectPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        AppPriority = par("AppPrioirty");
        commandIndex = 0;
        const char *script = par("sendScript");
        parseScript(script);
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("sendTimer");
    }
}

void HiUdpApp::handleStartOperation(LifecycleOperation *operation)
{
    simtime_t start = std::max(startTime, simTime());
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress =par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    setSocketOptions();
    if (startTime >= SIMTIME_ZERO && start <= stopTime ) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
}

void HiUdpApp::handleStopOperation(LifecycleOperation *operation)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
}

void HiUdpApp::handleCrashOperation(LifecycleOperation *operation)
{
    if (selfMsg)
        cancelEvent(selfMsg);
}

void HiUdpApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case SEND:
                processSend();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else
        socket.processMessage(msg);
}

void HiUdpApp::processStart()
{
    const char *destAddrs = par("connectAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        connectAddresses.push_back(result);
    }

    if (!connectAddresses.empty()) {
        selfMsg->setKind(SEND);
        scheduleAt(simTime(), selfMsg);
    }
    else {
        if (stopTime >= SIMTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleAt(stopTime, selfMsg);
        }
    }
}

void HiUdpApp::setSocketOptions()
{
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket.setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket.setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        NetworkInterface *ie = ift->findInterfaceByName(multicastInterface);
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
    if (joinLocalMulticastGroups) {
        MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
    }
    socket.setCallback(this);
}

void HiUdpApp::processSend()
{
    updateNextFlow(trafficMode);
    sendPacket();
    simtime_t txTime = simtime_t((packetLength+66)*8/linkSpeed);
    //66=8(UDP)+20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
    simtime_t d = simTime() + txTime/workLoad;
    EV<<"simTime() = "<<simTime()<<", next time = "<<d<<endl;
    if ((stopTime < SIMTIME_ZERO || d < stopTime)) {
        selfMsg->setKind(SEND);
        scheduleAt(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void HiUdpApp::sendPacket()
{
    srand(flowid);
    int k = rand()%connectAddresses.size();
    EV<<"dest seed k = "<<k<<endl;
    L3Address destAddr = chooseDestAddr(k);
    EV<<"Size of DestAddresses is "<<connectAddresses.size()<<endl;

    std::ostringstream str;
    str << packetName << "-" << flowid;

    Packet *packet = new Packet(str.str().c_str());

    auto payload = makeShared<ByteCountChunk>(B(packetLength));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(flowid);
    tag->setFlowSize(packetLength);
    tag->setCreationtime(simTime());
    tag->setPriority(AppPriority);

    packet->insertAtBack(payload);

    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, connectPort);
    flowid++;
    numSent++;
    BytesSent+=packetLength;
}

L3Address HiUdpApp::chooseDestAddr(int k)
{
    if (connectAddresses[k].isUnspecified() || connectAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), connectAddresses[k]);
    }
    return connectAddresses[k];
}

void HiUdpApp::processPacket(Packet *pck)
{
    auto addressReq = pck->addTagIfAbsent<L3AddressInd>();
    L3Address this_flow_src = addressReq->getSrcAddress();

    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        this_flow_id = region.getTag()->getFlowId();
        this_flow_creation_time = region.getTag()->getCreationtime();
        EV << "this_flow_id = "<<this_flow_id<<", this_flow_creation_time = "<<this_flow_creation_time<<"s"<<endl;
    }

    auto appbitpersec = pck->getBitLength() / (simTime()-last_pck_time).dbl();
    goodputVector.recordWithTimestamp(last_pck_time, appbitpersec);

    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pck) << endl;

    if (last_flow_creation_time != this_flow_creation_time)
    {
        numReceived++;
        flow_completion_time[numReceived-1] = last_pck_time - last_flow_creation_time;
        FCT_Vector.record(flow_completion_time[numReceived-1]);
        sumFct+=flow_completion_time[numReceived-1];
        EV << "newflow, last_flow_completion_time = "<<flow_completion_time[numReceived-1]<<"s, flowid = "<<last_flow_id<<endl;
    }

    last_flow_creation_time = this_flow_creation_time;
    last_pck_time = simTime();
    last_flow_id=this_flow_id;
    BytesRcvd+=pck->getByteLength();

    emit(packetReceivedSignal, pck);
    delete pck;
}

void HiUdpApp::finish()
{
    recordScalar("average FCT",sumFct.dbl()/numReceived);
    recordScalar("Flows sent", numSent);
    recordScalar("Flows received", numReceived);
    recordScalar("BytesRcvd", BytesRcvd);
    recordScalar("BytesSent", BytesSent);
    recordScalar("final flow id", flowid);
    ApplicationBase::finish();
}

void HiUdpApp::processStop()
{
}

void HiUdpApp::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void HiUdpApp::parseScript(const char *script)
{
    // ------------------------------------------------------------------------
    // 从指定的脚本文件 或 ini字符串参数中读取流量模式
//    char* buffer = nullptr;
//    if(*script){
//        std::fstream SCRIPT_FILE;
//        int file_length;
//        SCRIPT_FILE.open(script);
//        if(!SCRIPT_FILE.is_open()){     // 脚本文件不存在，表示script指向ini文件配置的字符串参数
//            EV_INFO << "Script file not found!Traffic Pattern is configured by const string." << endl;
//            EV_INFO << script << endl;
//        }
//        else{                           // 脚本文件存在,取出文件内容，赋值给buffer，再传给script
//            SCRIPT_FILE.seekg(0, std::ios::end);    // 将文件指针定位到文件结束位置
//            file_length = SCRIPT_FILE.tellg();      // 根据文件指针当前位置，计算得到文件长度
//            SCRIPT_FILE.seekg(0, std::ios::beg);    // 将文件指针定位到文件开始位置
//            if(file_length == 0) script = "";
//            else{
//                buffer = new char[file_length];
//                SCRIPT_FILE.read(buffer, file_length);
//                script = buffer;
//            }
//            SCRIPT_FILE.close();
//        }
//    }
//    delete[] buffer;
    // else{} 为空时，script指向空串 ""，表示既未指定脚本文件路径、又没有指定ini字符串参数
    // ------------------------------------------------------------------------
    const char *s = script;

    EV_DEBUG << "parse script \"" << script << "\"\n";
    while (*s) {
        // parse time
        while (isspace(*s))
            s++;

        if (!*s || *s == ';')
            break;

        const char *s0 = s;
        simtime_t tSend = strtod(s, &const_cast<char *&>(s));

        if (s == s0)
            throw cRuntimeError("Syntax error in script: simulation time expected");

        // parse number of bytes
        while (isspace(*s))
            s++;

        if (!isdigit(*s))
            throw cRuntimeError("Syntax error in script: number of bytes expected");

        long numBytes = strtol(s, nullptr, 10);

        while (isdigit(*s))
            s++;

        // add command
        EV_DEBUG << " add command (" << tSend << "s, " << numBytes << "B)\n";
        commands.push_back(Command(tSend, numBytes));

        // skip delimiter
        while (isspace(*s))
            s++;

        if (!*s)
            break;

        if (*s != ';')
            throw cRuntimeError("Syntax error in script: separator ';' missing");

        s++;

        while (isspace(*s))
            s++;
    }

    EV_DEBUG << "parser finished\n";
}

void HiUdpApp::updateNextFlow(const char* TM)
{
    //double seed = uniform(0,1);
    srand(flowid);
    double seed = double(rand()%9999)/9999;
    //flowsize_seed += 999;
    EV<<"flow size seed = "<<seed<<endl;
    if (std::string(TM).find("CacheFollower") != std::string::npos)
    {// 50% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 29% 1mb~, average 701kb
        if (seed<=0.01)
        {
            packetLength = 70;
        }
        else if (seed<=0.015)
        {
            packetLength = rand()%(150-71) + 71;
        }
        else if (seed<=0.04)
        {
            packetLength = 150;
        }
        else if (seed<=0.08)
        {
            packetLength = rand()%(300-151) + 151;
        }
        else if (seed<=0.1)
        {
            packetLength = rand()%(350-301) + 301;
        }
        else if (seed<=0.19)
        {
            packetLength = 350;
        }
        else if (seed<=0.2)
        {
            packetLength = rand()%(450-351) + 351;
        }
        else if (seed<=0.28)
        {
            packetLength = rand()%(500-451) + 451;
        }
        else if (seed<=0.3)
        {
            packetLength = rand()%(600-501) + 501;
        }
        else if (seed<=0.35)
        {
            packetLength = rand()%(700-601) + 601;
        }
        else if (seed<=0.4)
        {
            packetLength = rand()%(1100-701) + 701;
        }
        else if (seed<=0.42)
        {
            packetLength = rand()%(2000-1101) + 1101;
        }
        else if (seed<=0.48)
        {
            packetLength = rand()%(10000-2001) + 2001;
        }
        else if (seed<=0.5)
        {
            packetLength = rand()%(30000-10001) + 10001;
        }
        else if (seed<=0.52)
        {
            packetLength = rand()%(100000-30001) + 30001;
        }
        else if (seed<=0.6)
        {
            packetLength = rand()%(200000-100001) + 100001;
        }
        else if (seed<=0.68)
        {
            packetLength = rand()%(400000-200001) + 200001;
        }
        else if (seed<=0.7)
        {
            packetLength = rand()%(600000-400001) + 400001;
        }
        else if (seed<=0.701)
        {
            packetLength = rand()%(15000-6001)*100 + 600001;
        }
        else if (seed<=0.8)
        {
            packetLength = rand()%(20000-15001)*100 + 1500001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(24000-20001)*100 + 2000001;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(30000-24001)*100 + 2400001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("DataMining") != std::string::npos)
    {// 78% 0~10kb, 5% 10~100kb, 8% 100kb~1mb, 9% 1mb~, average 7410kb
        if (seed<=0.8)
        {
            packetLength = rand()%(10000-101) + 101;
        }
        else if (seed<=0.8346)
        {
            packetLength = rand()%(15252-1001)*10 + 10001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(39054-15253)*10 + 152523;
        }
        else if (seed<=0.953846)
        {
            packetLength = rand()%(32235-3906)*100 + 390542;
        }
        else if (seed<=0.99)
        {
            packetLength = rand()%(10000-323)*10000 + 3223543;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(10000-1001)*100000 + 100000001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("WebServer") != std::string::npos)
    {// 63% 0~10kb, 18% 10~100kb, 19% 100kb~1mb, 0% 1mb~, average 64kb
        if (seed<=0.12)
        {
            packetLength = rand()%(300-151) + 151;
        }
        else if (seed<=0.2)
        {
            packetLength = 300;
        }
        else if (seed<=0.3)
        {
            packetLength = rand()%(1000-601) + 601;
        }
        else if (seed<=0.4)
        {
            packetLength = rand()%(2000-1001) + 1001;
        }
        else if (seed<=0.5)
        {
            packetLength = rand()%(3100-2001) + 2001;
        }
        else if (seed<=0.6)
        {
            packetLength = rand()%(6000-3101) + 3101;
        }
        else if (seed<=0.71)
        {
            packetLength = rand()%(20000-6001) + 6001;
        }
        else if (seed<=0.8)
        {
            packetLength = rand()%(6000-2001)*10 + 20001;
        }
        else if (seed<=0.82)
        {
            packetLength = rand()%(15000-6001)*10 + 60001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(30000-15001)*10 + 150001;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(50000-30001)*10 + 300001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if ((std::string(TM).find("WebSearch") != std::string::npos))
    {// 49% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 20% 1mb~ (big14000kb), average 1600kb
        if (seed<=0.15)
        {
            packetLength = 9000;
        }
        else if (seed<=0.2)
        {
            packetLength = rand()%(18582-9001) + 9001;
        }
        else if (seed<=0.3)
        {
            packetLength = rand()%(28140-18583) + 18583;
        }
        else if (seed<=0.4)
        {
            packetLength = rand()%(38913-28141) + 28141;
        }
        else if (seed<=0.53)
        {
            packetLength = rand()%(7747-3892)*10 + 38914;
        }
        else if (seed<=0.6)
        {
            packetLength = rand()%(20000-7747)*10 + 77469;
        }
        else if (seed<=0.7)
        {
            packetLength = rand()%(10000-2001)*100 + 200001;
        }
        else if (seed<=0.8)
        {
            packetLength = rand()%(20000-10001)*100 + 1000001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(50000-20001)*100 + 2000001;
        }
        else if (seed<=0.97)
        {
            packetLength = rand()%(10000-5001)*1000 + 5000001;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(30000-10001)*1000 + 10000001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCep") != std::string::npos)
    {
        if (seed<=0.4436)
        {
            packetLength = 48;
        }
        else if (seed<=0.7265)
        {
            packetLength = 56;
        }
        else if (seed<=1)
        {
            packetLength = 128;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCcg") != std::string::npos)
    {
        if (seed<=0.6316)
        {
            packetLength = 48;
        }
        else if (seed<=0.6345)
        {
            packetLength = 128;
        }
        else if (seed<=0.8172)
        {
            packetLength = 3544;
        }
        else if (seed<=1)
        {
            packetLength = 3552;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCft") != std::string::npos)
    {
        if (seed<=0.0226)
        {
            packetLength = 48;
        }
        else if (seed<=0.0234)
        {
            packetLength = 56;
        }
        else if (seed<=0.0242)
        {
            packetLength = 64;
        }
        else if (seed<=0.025)
        {
            packetLength = 80;
        }
        else if (seed<=0.0258)
        {
            packetLength = 112;
        }
        else if (seed<=0.9115)
        {
            packetLength = 128;
        }
        else if (seed<=0.9123)
        {
            packetLength = 176;
        }
        else if (seed<=0.9131)
        {
            packetLength = 304;
        }
        else if (seed<=0.921)
        {
            packetLength = 308;
        }
        else if (seed<=0.9218)
        {
            packetLength = 560;
        }
        else if (seed<=0.9889)
        {
            packetLength = 1072;
        }
        else if (seed<=0.9697)
        {
            packetLength = 2096;
        }
        else if (seed<=0.9905)
        {
            packetLength = 4144;
        }
        else if (seed<=1)
        {
            packetLength = 16432;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCmg") != std::string::npos)
    {
        if (seed<=0.0978)
        {
            packetLength = 48;
        }
        else if (seed<=0.4065)
        {
            packetLength = 56;
        }
        else if (seed<=0.6638)
        {
            packetLength = 64;
        }
        else if (seed<=0.6747)
        {
            packetLength = 72;
        }
        else if (seed<=0.6954)
        {
            packetLength = 80;
        }
        else if (seed<=0.6980)
        {
            packetLength = 96;
        }
        else if (seed<=0.7180)
        {
            packetLength = 112;
        }
        else if (seed<=0.7316)
        {
            packetLength = 120;
        }
        else if (seed<=0.7445)
        {
            packetLength = 128;
        }
        else if (seed<=0.7664)
        {
            packetLength = 144;
        }
        else if (seed<=0.7873)
        {
            packetLength = 176;
        }
        else if (seed<=0.7900)
        {
            packetLength = 192;
        }
        else if (seed<=0.8109)
        {
            packetLength = 304;
        }
        else if (seed<=0.8302)
        {
            packetLength = 336;
        }
        else if (seed<=0.8495)
        {
            packetLength = 368;
        }
        else if (seed<=0.8688)
        {
            packetLength = 848;
        }
        else if (seed<=0.8881)
        {
            packetLength = 1072;
        }
        else if (seed<=0.9074)
        {
            packetLength = 1200;
        }
        else if (seed<=0.9267)
        {
            packetLength = 2640;
        }
        else if (seed<=0.9511)
        {
            packetLength = 4144;
        }
        else if (seed<=0.9756)
        {
            packetLength = 4400;
        }
        else if (seed<=1)
        {
            packetLength = 9296;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if(std::string(TM).find("LongFlow") != std::string::npos)
    {
        packetLength=10000;
    }
    else if(std::string(TM).find("sendscript") != std::string::npos){
        packetLength = commands[commandIndex].numBytes;
    }
    else
    {
        throw cRuntimeError("Unrecognized traffic mode!");
    }
    EV<<"update new flow, traffic mode = "<<TM<<", new flow size = "<<packetLength<<endl;
}

void HiUdpApp::socketClosed(UdpSocket *socket)
{
    if (operationalState == State::STOPPING_OPERATION)
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void HiUdpApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void HiUdpApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}


} // namespace inet

