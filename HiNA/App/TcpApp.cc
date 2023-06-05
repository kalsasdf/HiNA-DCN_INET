//
// Copyright (C) 2004 Andras Varga
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

#include "TcpApp.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"

namespace inet {

Define_Module(TcpApp);

#define MSGKIND_CONNECT    1
#define MSGKIND_SEND       2
#define MSGKIND_CLOSE      3

TcpApp::~TcpApp()
{
    cancelAndDelete(selfMsg);
}

void TcpApp::initialize(int stage)
{
    TcpAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        goodputVector.setName("goodput (bps)");
        WATCH(numSent);
        WATCH(numReceived);

        tOpen = par("tOpen");
        tSend = par("tSend");
        tClose = par("tClose");
        packetName = par("packetName");
        workLoad = par("workLoad");
        linkSpeed = par("linkSpeed");
        trafficMode = par("trafficMode");
        AppPriority = par("AppPriority");
        longflow = par("longflow");
        commandIndex = 0;

        const char *script = par("sendScript");
        parseScript(script);

        selfMsg = new cMessage("selftimer");
    }else if (stage == INITSTAGE_APPLICATION_LAYER) {
        if(tOpen<SIMTIME_ZERO)
            socket.listen();
    }
}

void TcpApp::handleStartOperation(LifecycleOperation *operation)
{
    if (tOpen>=SIMTIME_ZERO&&tOpen<tClose) {EV<<"handleStartOperation"<<endl;
        selfMsg->setKind(MSGKIND_CONNECT);
        scheduleAt(tOpen, selfMsg);
    }
}

void TcpApp::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    if (socket.isOpen())
        close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void TcpApp::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    if (operation->getRootModule() != getContainingNode(this)){
        socket.destroy();
    }
}

void TcpApp::handleTimer(cMessage *msg)
{
    switch (msg->getKind()) {
        case MSGKIND_CONNECT:
            connect(); // sending will be scheduled from socketEstablished()
            break;

        case MSGKIND_SEND:
            sendData();
            break;

        case MSGKIND_CLOSE:
            close();
            break;

        default:
            throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
}

void TcpApp::socketEstablished(TcpSocket *socket)
{
    TcpAppBase::socketEstablished(socket);

    ASSERT(commandIndex == 0);
    if(tOpen>=0){
        selfMsg->setKind(MSGKIND_SEND);
        scheduleAt(std::max(tSend, simTime()), selfMsg);
    }

}

void TcpApp::sendData()
{
    updateNextFlow(trafficMode);
    EV_INFO << "sending data with " << packetLength << " bytes\n";
    sendPacket(createDataPacket(packetLength));
    if(simTime()>=tClose){
        selfMsg->setKind(MSGKIND_CLOSE);
        scheduleAt(std::max(tClose, simTime()), selfMsg);
    }else if(std::string(trafficMode).find("LongFlow") != std::string::npos){
        simtime_t txTime = simtime_t((packetLength+ceil(packetLength/1460)*78)*8/linkSpeed);
        //78=20(TCP)+20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
        simtime_t d = simTime() + txTime/workLoad;
        EV<<"simTime() = "<<simTime()<<", next time = "<<d<<endl;
        selfMsg->setKind(MSGKIND_SEND);
        scheduleAt(d, selfMsg);
    }else if(++commandIndex < (int)commands.size()){
        simtime_t tSend = commands[commandIndex].tSend;
        selfMsg->setKind(MSGKIND_SEND);
        scheduleAt(std::max(tSend, simTime()), selfMsg);
    }

}

Packet *TcpApp::createDataPacket(long sendBytes)
{
    std::ostringstream str;
    str << packetName;
    Packet *packet = new Packet(str.str().c_str());
    auto payload = makeShared<ByteCountChunk>(B(sendBytes));
    auto tag=payload->addTag<HiTag>();
    tag->setPriority(AppPriority);
    packet->insertAtBack(payload);
    numSent++;
    BytesSent+=sendBytes;
    return packet;
}

void TcpApp::socketDataArrived(TcpSocket *socket, Packet *pck, bool urgent)
{
    auto appbitpersec = pck->getBitLength() / (simTime()-last_pck_time).dbl();
    goodputVector.recordWithTimestamp(last_pck_time, appbitpersec);

    last_pck_time = simTime();
    BytesRcvd+=pck->getByteLength();
    TcpAppBase::socketDataArrived(socket, pck, urgent);
}

void TcpApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleTimer(msg);
    else {
        TcpSocket *temp_socket = check_and_cast_nullable<TcpSocket *>(socketMap.findSocketFor(msg));
        if (temp_socket){
            EV<<"TcpApp::handleMessageWhenUp, temp_socket is true"<<endl;
            temp_socket->processMessage(msg);
        }
        else if (socket.belongsToSocket(msg)){
            EV<<"TcpApp::handleMessageWhenUp, temp_socket is false"<<endl;
            socket.processMessage(msg);
        }
        else {;
            EV_ERROR << "message " << msg->getFullName() << "(" << msg->getClassName() << ") arrived for unknown socket \n";
            delete msg;
        }
    }
}

void TcpApp::finish()
{
    recordScalar("num sent", numSent);
    recordScalar("num received", numReceived);
    recordScalar("BytesRcvd", BytesRcvd);
    recordScalar("BytesSent", BytesSent);
}

void TcpApp::socketAvailable(TcpSocket *socket, TcpAvailableInfo *availableInfo)
{
    // new TCP connection -- create new socket object and server process
    TcpSocket *newSocket = new TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut"));
    socketMap.addSocket(newSocket);
    socket->accept(availableInfo->getNewSocketId());
}

void TcpApp::socketClosed(TcpSocket *socket)
{
    TcpAppBase::socketClosed(socket);
    cancelEvent(selfMsg);
    if (operationalState == State::STOPPING_OPERATION && !this->socket.isOpen())
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void TcpApp::socketFailure(TcpSocket *socket, int code)
{
    TcpAppBase::socketFailure(socket, code);
    cancelEvent(selfMsg);
}

void TcpApp::refreshDisplay() const
{
    TcpAppBase::refreshDisplay();
    std::ostringstream os;
    os << TcpSocket::stateName(socket.getState()) << "\nsent: " << BytesSent << " bytes\nrcvd: " << BytesRcvd << " bytes";
    getDisplayString().setTagArg("t", 0, os.str().c_str());
}

void TcpApp::parseScript(const char *script)
{
    // ------------------------------------------------------------------------
    // 从指定的脚本文件 或 ini字符串参数中读取流量模式
//    char* buffer = nullptr;
//    if(*script){
//        std::fstream SCRIPT_FILE;
//        int file_length;
//        SCRIPT_FILE.open(script);
//        if(!SCRIPT_FILE.is_open()){     // 脚本文件不存在，表示script指向ini文件配置的字符串参数
//            EV_INFO << "Script file not found! Traffic Pattern is configured by const string." << endl;
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
    //    delete[] buffer;
    EV << "parser finished\n";
}

void TcpApp::updateNextFlow(const char* TM)
{
    if(std::string(TM).find("sendscript") != std::string::npos){
        packetLength = commands[commandIndex].numBytes;
    }else if(std::string(TM).find("LongFlow") != std::string::npos){
        packetLength=longflow;
    }
    else
    {
        throw cRuntimeError("Unrecognized traffic mode!");
    }
}
} // namespace inet

