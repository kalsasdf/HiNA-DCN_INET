//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package inet.HiNA.Ethernet.HiEthernetEncap;


import inet.linklayer.ethernet.contract.IEthernetLayer;
import inet.linklayer.ethernet.basic.EthernetEncapsulation;


//
simple HiEthernetEncapsulation like IEthernetLayer
{
    parameters:
        bool registerProtocol = default(false);     //FIXME //KLUDGE should redesign place of EthernetEncapsulation and LLC modules
        string interfaceTableModule;   // The path to the InterfaceTable module
        string fcsMode @enum("declared","computed") = default("declared");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation
        @class(EthernetEncapsulation);
        @display("i=block/layer");
        @signal[encapPk](type=cPacket);
        @signal[decapPk](type=cPacket);
        @signal[pauseSent](type=long); // pause units
        @signal[packetDropped](type=inet::Packet);
        @statistic[encapPk](title="packets encapsulated"; source=encapPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[decapPk](title="packets decapsulated"; source=decapPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[pauseSent](title="pause sent"; record=count,sum,vector);
    gates:
        input upperLayerIn; // TODO accepts packets and socket commands with  @messageKinds(inet::SocketCommandCode), too
        output upperLayerOut; // TODO sends packets and socket indications with @messageKinds(inet::SocketStatusInd), too
        input lowerLayerIn;
        output lowerLayerOut;
}

