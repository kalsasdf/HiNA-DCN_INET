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

package inet.HiNA.Ethernet.BCNMac;

import inet.linklayer.contract.IEtherMac;


//
simple BCNMac like IEtherMac
{
    parameters:
        string interfaceTableModule;        // The path to the InterfaceTable module
        string displayStringTextFormat = default("rate: %b\nsent: %s, rcvd: %r\nqueue: %q, drop: %d");
        bool sendRawBytes = default(false); // when true packets are serialized into a sequence of bytes before sending out
        bool promiscuous = default(false);  // if true, all packets are received, otherwise only the
                                            // ones with matching destination MAC address
        bool duplexMode;                    // must be set to "true", as EtherMacFullDuplex does not support half-duplex operation
                                            // (parameter is present to reduce the risk of accidental misconfiguration)
        int mtu @unit(B) = default(1500B);
        string fcsMode @enum("declared","computed") = default("declared");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation
        int batchSize = default(50);
        double maxInterval @unit(s) = default(1s);
        bool TIMELY = default(false);
        bool HPCC = default(false);
       
        @class(BCNMac);
        @display("i=block/rxtx");

        @signal[txPk](type=inet::Packet);
        @signal[rxPkOk](type=inet::Packet);
        @signal[txPausePkUnits](type=long);
        @signal[rxPausePkUnits](type=long);
        @signal[rxPkFromHl](type=inet::Packet);
        @signal[packetDropped](type=inet::Packet);
        @signal[packetSentToLower](type=inet::Packet);
        @signal[packetReceivedFromLower](type=inet::Packet);
        @signal[packetSentToUpper](type=inet::Packet);
        @signal[packetReceivedFromUpper](type=inet::Packet);
        @signal[transmissionStateChanged](type=long); // enum=MacTransmitState
        @signal[receptionStateChanged](type=long); // enum=MacReceiveState
        @signal[transmissionStarted](type=inet::physicallayer::EthernetSignalBase);
        @signal[transmissionEnded](type=inet::physicallayer::EthernetSignalBase);
        @signal[receptionEnded](type=inet::physicallayer::EthernetSignalBase);
        //when mac receive pfc frame, emit signal to scheduler;
        //not for statistic, the function of statistic in mac.cc--> finish()
        @signal[bfcpauseframe](type=inet::EthernetControlFrameBase);
        @signal[bfcresumeframe](type=inet::EthernetControlFrameBase);

        @statistic[txPk](title="packets transmitted"; source=packetSentToLower; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[rxPkOk](title="packets received OK"; source=rxPkOk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[passedUpPk](title="packets passed to higher layer"; source=packetSentToUpper; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[txPausePkUnits](title="pause units sent"; record=count,sum,vector; interpolationmode=none);
        @statistic[rxPausePkUnits](title="pause units received"; record=count,sum,vector; interpolationmode=none);
        @statistic[packetReceivedFromUpper](title="packet bytes from higher layer"; source=packetReceivedFromUpper; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[packetDropIncorrectlyReceived](title="packet drop: incorrectly received"; source=packetDropReasonIsIncorrectlyReceived(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropInterfaceDown](title="packet drop: interface down"; source=packetDropReasonIsInterfaceDown(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropNotAddressedToUs](title="packet drop: not addressed to us"; source=packetDropReasonIsNotAddressedToUs(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
		
		
		
    gates:
        input upperLayerIn @labels(EtherFrame);    // to ~EtherLlc or ~EtherEncap or ~IMacRelayUnit
        output upperLayerOut @labels(EtherFrame);  // to ~EtherLlc or ~EtherEncap or ~IMacRelayUnit
        inout phys @labels(EtherFrame);            // to physical layer or the network
}

