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


HiUdpApp::~HiUdpApp()
{
    cancelAndDelete(selfMsg);
}

void HiUdpApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {EV<<"HiUdpApp::initialize stage = "<<stage<<endl;
        FCT_Vector.setName("FCT");
        shortflow_FCT_Vector.setName("shortflow_FCT");
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
        AppPriority = par("AppPriority");
        longflow = par("longflow");
        Enablepoisson = par("Enablepoisson");
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
    if(count==0){
        updateNextFlow(trafficMode);
        auto module = check_and_cast<GlobalFlowId*>(getParentModule()->getSubmodule("GlobalFlowId"));
        flowid = module->getflowid();
        module->flowidadd();
        if(messageLength>65527){
            count = num = messageLength/65527;
            sendPacket(65527,flowid);
            selfMsg->setKind(SEND);
            scheduleAt(simTime(), selfMsg);
        }else{
            sendPacket(messageLength,flowid);
            simtime_t txTime = simtime_t((messageLength+ceil(double(messageLength)/1472)*66)*8/linkSpeed);EV<<"txTime = "<<txTime<<endl;
            if(Enablepoisson) txTime = exponential(txTime);
            //66=8(UDP)+20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
            simtime_t d = simTime() + txTime/workLoad;EV<<"poissontime = "<<txTime<<endl;
            if(std::string(trafficMode).find("sendscript") != std::string::npos&&++commandIndex < (int)commands.size()){
                simtime_t tSend = commands[commandIndex].tSend;
                selfMsg->setKind(SEND);
                scheduleAt(std::max(tSend, simTime()), selfMsg);
            }else if(std::string(trafficMode).find("sendscript") == std::string::npos&&d<stopTime){
                EV<<"simTime() = "<<simTime()<<", next time = "<<d<<endl;
                selfMsg->setKind(SEND);
                scheduleAt(d, selfMsg);
            }else {
                selfMsg->setKind(STOP);
                scheduleAt(stopTime, selfMsg);
            }
        }
    }else{
        count--;
        if(count==0){
            sendPacket(messageLength-num*65527,flowid);
            simtime_t txTime = simtime_t((messageLength+ceil(double(messageLength)/1472)*66)*8/linkSpeed);EV<<"txTime = "<<txTime<<endl;
            if(Enablepoisson) txTime = exponential(txTime);
            //66=8(UDP)+20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
            simtime_t d = simTime() + txTime/workLoad;EV<<"poissontime = "<<txTime<<endl;
            if(std::string(trafficMode).find("sendscript") != std::string::npos&&++commandIndex < (int)commands.size()){
                simtime_t tSend = commands[commandIndex].tSend;
                selfMsg->setKind(SEND);
                scheduleAt(std::max(tSend, simTime()), selfMsg);
            }else if(d<stopTime){
                EV<<"simTime() = "<<simTime()<<", next time = "<<d<<endl;
                selfMsg->setKind(SEND);
                scheduleAt(d, selfMsg);
            }else {
                selfMsg->setKind(STOP);
                scheduleAt(stopTime, selfMsg);
            }
        }else{
            sendPacket(65527,flowid);
            selfMsg->setKind(SEND);
            scheduleAt(simTime(), selfMsg);
        }
    }
}

void HiUdpApp::sendPacket(int packetlength, uint64_t flowid)
{
    srand(flowid);
    int k = rand()%connectAddresses.size();
    EV<<"dest seed k = "<<k<<endl;
    L3Address destAddr = chooseDestAddr(k);
    EV<<"destAddr is "<<destAddr<<endl;

    std::ostringstream str;
    str << packetName << "-" << flowid;

    Packet *packet = new Packet(str.str().c_str());

    auto payload = makeShared<ByteCountChunk>(B(packetlength));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(flowid);
    tag->setFlowSize(messageLength);
    tag->setCreationtime(simTime());
    tag->setPriority(AppPriority);

    packet->insertAtBack(payload);

    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, connectPort);
    numSent++;
    BytesSent+=packetlength;
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
    auto appbitpersec = pck->getBitLength() / (simTime()-last_pck_time).dbl();
    goodputVector.recordWithTimestamp(simTime(), appbitpersec);
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pck) << endl;

    bool last = false;
    uint64_t this_flow_id;
    simtime_t this_flow_creation_time;
    long flowsize;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        this_flow_id = region.getTag()->getFlowId();
        this_flow_creation_time = region.getTag()->getCreationtime();
        last = region.getTag()->isLastPck();
        flowsize = region.getTag()->getFlowSize();
    }
    EV << "this_flow_id = "<<this_flow_id<<", this_flow_creation_time = "<<this_flow_creation_time<<"s"<<", last = "<<last<<endl;

    if (last)
    {
        flow_completion_time[numReceived] = simTime() - this_flow_creation_time;
        FCT_Vector.record(flow_completion_time[numReceived]);
        if(flowsize<=1e5) shortflow_FCT_Vector.record(flow_completion_time[numReceived]);
        sumFct+=flow_completion_time[numReceived];
        EV << "flow ends, this_flow_creation_time = "<<this_flow_creation_time<<"s, fct = "<<flow_completion_time[numReceived]<<"s, flowid = "<<this_flow_id<<endl;
        numReceived++;
    }
    last_pck_time = simTime();
    BytesRcvd+=pck->getByteLength();
    delete pck;
}

void HiUdpApp::finish()
{
    recordScalar("average FCT",sumFct.dbl()/numReceived);
    recordScalar("Flows sent", numSent);
    recordScalar("Flows received", numReceived);
    recordScalar("BytesRcvd", BytesRcvd);
    recordScalar("BytesSent", BytesSent);
//    recordScalar("final flow id", flowid);
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
    char* buffer = nullptr;
    if(*script){
        std::fstream SCRIPT_FILE;
        int file_length;
        SCRIPT_FILE.open(script);
        if(!SCRIPT_FILE.is_open()){     // 脚本文件不存在，表示script指向ini文件配置的字符串参数
            EV_INFO << "Script file not found!Traffic Pattern is configured by const string." << endl;
            EV_INFO << script << endl;
        }
        else{                           // 脚本文件存在,取出文件内容，赋值给buffer，再传给script
            SCRIPT_FILE.seekg(0, std::ios::end);    // 将文件指针定位到文件结束位置
            file_length = SCRIPT_FILE.tellg();      // 根据文件指针当前位置，计算得到文件长度
            SCRIPT_FILE.seekg(0, std::ios::beg);    // 将文件指针定位到文件开始位置
            if(file_length == 0) script = "";
            else{
                buffer = new char[file_length];
                SCRIPT_FILE.read(buffer, file_length);
                script = buffer;
            }
            SCRIPT_FILE.close();
        }
    }
    // else{} 为空时，script指向空串 ""，表示既未指定脚本文件路径、又没有指定ini字符串参数
    // ------------------------------------------------------------------------
    const char *s = script;

    EV << "parse script \"" << script << "\"\n";
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
        EV << " add command (" << tSend << "s, " << numBytes << "B)\n";
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
        delete[] buffer;
    EV << "parser finished\n";
}

void HiUdpApp::updateNextFlow(const char* TM)
{
    //double seed = uniform(0,1);
//    srand(flowid);
    double seed = double(rand()%9999)/9999;
    //flowsize_seed += 999;
    EV<<"flow size seed = "<<seed<<endl;
    if (std::string(TM).find("CacheFollower") != std::string::npos)
    {// 50% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 29% 1mb~, average 701kb
        if (seed<=0.01)
        {
            messageLength = 70;
        }
        else if (seed<=0.015)
        {
            messageLength = rand()%(150-71) + 71;
        }
        else if (seed<=0.04)
        {
            messageLength = 150;
        }
        else if (seed<=0.08)
        {
            messageLength = rand()%(300-151) + 151;
        }
        else if (seed<=0.1)
        {
            messageLength = rand()%(350-301) + 301;
        }
        else if (seed<=0.19)
        {
            messageLength = 350;
        }
        else if (seed<=0.2)
        {
            messageLength = rand()%(450-351) + 351;
        }
        else if (seed<=0.28)
        {
            messageLength = rand()%(500-451) + 451;
        }
        else if (seed<=0.3)
        {
            messageLength = rand()%(600-501) + 501;
        }
        else if (seed<=0.35)
        {
            messageLength = rand()%(700-601) + 601;
        }
        else if (seed<=0.4)
        {
            messageLength = rand()%(1100-701) + 701;
        }
        else if (seed<=0.42)
        {
            messageLength = rand()%(2000-1101) + 1101;
        }
        else if (seed<=0.48)
        {
            messageLength = rand()%(10000-2001) + 2001;
        }
        else if (seed<=0.5)
        {
            messageLength = rand()%(30000-10001) + 10001;
        }
        else if (seed<=0.52)
        {
            messageLength = rand()%(100000-30001) + 30001;
        }
        else if (seed<=0.6)
        {
            messageLength = rand()%(200000-100001) + 100001;
        }
        else if (seed<=0.68)
        {
            messageLength = rand()%(400000-200001) + 200001;
        }
        else if (seed<=0.7)
        {
            messageLength = rand()%(600000-400001) + 400001;
        }
        else if (seed<=0.701)
        {
            messageLength = rand()%(15000-6001)*100 + 600001;
        }
        else if (seed<=0.8)
        {
            messageLength = rand()%(20000-15001)*100 + 1500001;
        }
        else if (seed<=0.9)
        {
            messageLength = rand()%(24000-20001)*100 + 2000001;
        }
        else if (seed<=1)
        {
            messageLength = rand()%(30000-24001)*100 + 2400001;
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
            messageLength = rand()%(10000-101) + 101;
        }
        else if (seed<=0.8346)
        {
            messageLength = rand()%(15252-1001)*10 + 10001;
        }
        else if (seed<=0.9)
        {
            messageLength = rand()%(39054-15253)*10 + 152523;
        }
        else if (seed<=0.953846)
        {
            messageLength = rand()%(32235-3906)*100 + 390542;
        }
        else if (seed<=0.99)
        {
            messageLength = rand()%(10000-323)*10000 + 3223543;
        }
        else if (seed<=1)
        {
            messageLength = rand()%(10000-1001)*100000 + 100000001;
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
            messageLength = rand()%(300-151) + 151;
        }
        else if (seed<=0.2)
        {
            messageLength = 300;
        }
        else if (seed<=0.3)
        {
            messageLength = rand()%(1000-601) + 601;
        }
        else if (seed<=0.4)
        {
            messageLength = rand()%(2000-1001) + 1001;
        }
        else if (seed<=0.5)
        {
            messageLength = rand()%(3100-2001) + 2001;
        }
        else if (seed<=0.6)
        {
            messageLength = rand()%(6000-3101) + 3101;
        }
        else if (seed<=0.71)
        {
            messageLength = rand()%(20000-6001) + 6001;
        }
        else if (seed<=0.8)
        {
            messageLength = rand()%(6000-2001)*10 + 20001;
        }
        else if (seed<=0.82)
        {
            messageLength = rand()%(15000-6001)*10 + 60001;
        }
        else if (seed<=0.9)
        {
            messageLength = rand()%(30000-15001)*10 + 150001;
        }
        else if (seed<=1)
        {
            messageLength = rand()%(50000-30001)*10 + 300001;
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
            messageLength = 9000;
        }
        else if (seed<=0.2)
        {
            messageLength = rand()%(18582-9001) + 9001;
        }
        else if (seed<=0.3)
        {
            messageLength = rand()%(28140-18583) + 18583;
        }
        else if (seed<=0.4)
        {
            messageLength = rand()%(38913-28141) + 28141;
        }
        else if (seed<=0.53)
        {
            messageLength = rand()%(7747-3892)*10 + 38914;
        }
        else if (seed<=0.6)
        {
            messageLength = rand()%(20000-7747)*10 + 77469;
        }
        else if (seed<=0.7)
        {
            messageLength = rand()%(10000-2001)*100 + 200001;
        }
        else if (seed<=0.8)
        {
            messageLength = rand()%(20000-10001)*100 + 1000001;
        }
        else if (seed<=0.9)
        {
            messageLength = rand()%(50000-20001)*100 + 2000001;
        }
        else if (seed<=0.97)
        {
            messageLength = rand()%(10000-5001)*1000 + 5000001;
        }
        else if (seed<=1)
        {
            messageLength = rand()%(30000-10001)*1000 + 10000001;
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
            messageLength = 48;
        }
        else if (seed<=0.7265)
        {
            messageLength = 56;
        }
        else if (seed<=1)
        {
            messageLength = 128;
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
            messageLength = 48;
        }
        else if (seed<=0.6345)
        {
            messageLength = 128;
        }
        else if (seed<=0.8172)
        {
            messageLength = 3544;
        }
        else if (seed<=1)
        {
            messageLength = 3552;
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
            messageLength = 48;
        }
        else if (seed<=0.0234)
        {
            messageLength = 56;
        }
        else if (seed<=0.0242)
        {
            messageLength = 64;
        }
        else if (seed<=0.025)
        {
            messageLength = 80;
        }
        else if (seed<=0.0258)
        {
            messageLength = 112;
        }
        else if (seed<=0.9115)
        {
            messageLength = 128;
        }
        else if (seed<=0.9123)
        {
            messageLength = 176;
        }
        else if (seed<=0.9131)
        {
            messageLength = 304;
        }
        else if (seed<=0.921)
        {
            messageLength = 308;
        }
        else if (seed<=0.9218)
        {
            messageLength = 560;
        }
        else if (seed<=0.9889)
        {
            messageLength = 1072;
        }
        else if (seed<=0.9697)
        {
            messageLength = 2096;
        }
        else if (seed<=0.9905)
        {
            messageLength = 4144;
        }
        else if (seed<=1)
        {
            messageLength = 16432;
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
            messageLength = 48;
        }
        else if (seed<=0.4065)
        {
            messageLength = 56;
        }
        else if (seed<=0.6638)
        {
            messageLength = 64;
        }
        else if (seed<=0.6747)
        {
            messageLength = 72;
        }
        else if (seed<=0.6954)
        {
            messageLength = 80;
        }
        else if (seed<=0.6980)
        {
            messageLength = 96;
        }
        else if (seed<=0.7180)
        {
            messageLength = 112;
        }
        else if (seed<=0.7316)
        {
            messageLength = 120;
        }
        else if (seed<=0.7445)
        {
            messageLength = 128;
        }
        else if (seed<=0.7664)
        {
            messageLength = 144;
        }
        else if (seed<=0.7873)
        {
            messageLength = 176;
        }
        else if (seed<=0.7900)
        {
            messageLength = 192;
        }
        else if (seed<=0.8109)
        {
            messageLength = 304;
        }
        else if (seed<=0.8302)
        {
            messageLength = 336;
        }
        else if (seed<=0.8495)
        {
            messageLength = 368;
        }
        else if (seed<=0.8688)
        {
            messageLength = 848;
        }
        else if (seed<=0.8881)
        {
            messageLength = 1072;
        }
        else if (seed<=0.9074)
        {
            messageLength = 1200;
        }
        else if (seed<=0.9267)
        {
            messageLength = 2640;
        }
        else if (seed<=0.9511)
        {
            messageLength = 4144;
        }
        else if (seed<=0.9756)
        {
            messageLength = 4400;
        }
        else if (seed<=1)
        {
            messageLength = 9296;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if(std::string(TM).find("LongFlow") != std::string::npos)
    {
        messageLength=longflow;
    }
    else if(std::string(TM).find("sendscript") != std::string::npos){
        messageLength = commands[commandIndex].numBytes;
    }
    else
    {
        throw cRuntimeError("Unrecognized traffic mode!");
    }
    EV<<"update new flow, traffic mode = "<<TM<<", new flow size = "<<messageLength<<endl;
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

