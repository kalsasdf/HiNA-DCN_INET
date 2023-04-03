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

uint32_t TcpApp::flowid=0;

TcpApp::~TcpApp()
{
    cancelAndDelete(timeoutMsg);
}

void TcpApp::initialize(int stage)
{
    TcpAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        FCT_Vector.setName("FCT");
        goodputVector.setName("goodput (bps)");
        WATCH(numSent);
        WATCH(numReceived);
        flow_completion_time.clear();

        tOpen = par("tOpen");
        tSend = par("tSend");
        tClose = par("tClose");
        packetName = par("packetName");
        sendBytes = par("sendBytes");
        commandIndex = 0;

        const char *script = par("sendScript");
        parseScript(script);

        if (sendBytes > 0 && commands.size() > 0)
            throw cRuntimeError("Cannot use both sendScript and tSend+sendBytes");
        if (sendBytes > 0)
            commands.push_back(Command(tSend, sendBytes));
        if (commands.size() == 0)
            throw cRuntimeError("sendScript is empty");

        timeoutMsg = new cMessage("timer");
    }else if (stage == INITSTAGE_APPLICATION_LAYER) {
        if(tOpen<SIMTIME_ZERO)
            socket.listen();
    }
}

void TcpApp::handleStartOperation(LifecycleOperation *operation)
{
    if (tOpen>=SIMTIME_ZERO&&tOpen<tClose) {EV<<"handleStartOperation"<<endl;
        timeoutMsg->setKind(MSGKIND_CONNECT);
        scheduleAt(tOpen, timeoutMsg);
    }
}

void TcpApp::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
    if (socket.isOpen())
        close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void TcpApp::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
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
        timeoutMsg->setKind(MSGKIND_SEND);
        simtime_t tSend = commands[commandIndex].tSend;
        scheduleAt(std::max(tSend, simTime()), timeoutMsg);
    }

}

void TcpApp::sendData()
{
    long numBytes;

    numBytes = commands[commandIndex].numBytes;

    EV_INFO << "sending data with " << numBytes << " bytes\n";
    sendPacket(createDataPacket(numBytes));
    if(simTime()<tClose&&++commandIndex < (int)commands.size()){
        simtime_t tSend = commands[commandIndex].tSend;
        timeoutMsg->setKind(MSGKIND_SEND);
        scheduleAt(std::max(tSend, simTime()), timeoutMsg);
    }
    else {
        timeoutMsg->setKind(MSGKIND_CLOSE);
        scheduleAt(std::max(tClose, simTime()), timeoutMsg);
    }

}

Packet *TcpApp::createDataPacket(long sendBytes)
{
    std::ostringstream str;
    str << packetName << "-" << flowid;
    Packet *packet = new Packet(str.str().c_str());
    auto payload = makeShared<ByteCountChunk>(B(sendBytes));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(flowid);
    tag->setFlowSize(sendBytes);
    tag->setCreationtime(simTime());
    EV_INFO<<"flow "<<flowid<<" creationtime = "<<simTime()<<endl;
    packet->insertAtBack(payload);
    flowid++;
    numSent++;
    BytesSent+=sendBytes;
    return packet;
}

void TcpApp::socketDataArrived(TcpSocket *socket, Packet *pck, bool urgent)
{
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        this_flow_id = region.getTag()->getFlowId();
        this_flow_creation_time = region.getTag()->getCreationtime();
    }

    auto appbitpersec = pck->getBitLength() / (simTime()-last_pck_time).dbl();
    goodputVector.recordWithTimestamp(last_pck_time, appbitpersec);

    EV_INFO<<"this_flow_id = "<<this_flow_id<<endl;//", source address = "<<addressReq->getSrcAddress()<<endl;
    EV_INFO<<"this_flow_creation_time = "<<this_flow_creation_time<<", last_pck_time = "<<last_pck_time<<endl;

    if (last_flow_creation_time != this_flow_creation_time)
    {
        numReceived++;
        flow_completion_time[numReceived-1] = last_pck_time - last_flow_creation_time;
        FCT_Vector.record(flow_completion_time[numReceived-1]);
        sumFct+=flow_completion_time[numReceived-1];
        EV << "socketDataArrived(), flow_transmission_time = "<<flow_completion_time[numReceived-1]<<" flowid = "<<last_flow_id<<endl;
    }

    last_flow_creation_time = this_flow_creation_time;
    last_pck_time = simTime();
    last_flow_id=this_flow_id;
    BytesRcvd+=pck->getByteLength();
    TcpAppBase::socketDataArrived(socket, pck, urgent);
}

void TcpApp::finish()
{

    recordScalar("average FCT",sumFct.dbl()/numReceived);
    recordScalar("Flows sent", numSent);
    recordScalar("Flows received", numReceived);
    recordScalar("BytesRcvd", BytesRcvd);
    recordScalar("BytesSent", BytesSent);
    recordScalar("final flow id", flowid);
}

void TcpApp::socketClosed(TcpSocket *socket)
{
    TcpAppBase::socketClosed(socket);
    cancelEvent(timeoutMsg);
    if (operationalState == State::STOPPING_OPERATION && !this->socket.isOpen())
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void TcpApp::socketFailure(TcpSocket *socket, int code)
{
    TcpAppBase::socketFailure(socket, code);
    cancelEvent(timeoutMsg);
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

} // namespace inet

