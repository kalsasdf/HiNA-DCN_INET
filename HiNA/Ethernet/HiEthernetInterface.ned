//
// Copyright (C) 2004 Andras Varga
//               2010 Zoltan Bojthe
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

package inet.HiNA.Ethernet;

import inet.common.IHook;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.queue.PacketQueue;
import inet.linklayer.contract.IEtherMac;
import inet.linklayer.contract.IEthernetInterface;
import inet.queueing.contract.ITrafficConditioner;
import inet.linklayer.ethernet.contract.IEthernetLayer;
import inet.HiNA.Ethernet.HiQueue.HiQueue;
import inet.HiNA.Ethernet.SwitchMacQueue.SwitchMacQueue;
import inet.HiNA.Ethernet.HiEthernetMac.HiEthernetMac;
import inet.networklayer.common.NetworkInterface;
import inet.queueing.queue.PacketQueue;

//
// Ethernet network interface. Complements ~EtherMac and ~EtherEncap
// with an output queue for QoS and RED support.
//
module HiEthernetInterface extends NetworkInterface like IEthernetInterface
{
    parameters:
        string interfaceTableModule;
        int numOutputHooks = default(0);
        int numInputHooks = default(0);
        string protocol = default("ethernetmac");
        string address @mutable = default("auto");   // MAC address as hex string (12 hex digits), or
                                                     // "auto". "auto" values will be replaced by
                                                     // a generated MAC address in init stage 0.
        bool duplexMode = default(true);      // selects full-duplex (true) or half-duplex (false) operation
        bool csmacdSupport = default(!duplexMode);  // by default CSMA/CD is turned off, so only point-to-point duplex links are supported.
        double bitrate @unit(bps) = default(0 bps);
        bool usePfc = default(false);
        *.usePfc = this.usePfc;
        bool useEcn = default(false);
        *.useEcn = this.useEcn;
        bool isSwitch = default(false);
        string fcsMode @enum("declared","computed") = default("declared");
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.fcsMode = this.fcsMode;
        *.duplexMode = this.duplexMode;
    gates:
        input cutthroughIn @loose;
        output cutthroughOut @loose;
        input upperLayerIn;
        output upperLayerOut;
        inout phys @labels(EthernetSignal);
    submodules:
        ingressTC: <default("")> like ITrafficConditioner {
            @display("p=573,200");
        }
        egressTC: <default("")> like ITrafficConditioner {
            @display("p=250,192");
        }
        Hiqueue: HiQueue if isSwitch {
            @display("p=334,331");
        }
        Hostqueue: PacketQueue if (!isSwitch) {
            parameters:
                @display("p=250,502;q=l2queue");
        }
        Switchqueue: SwitchMacQueue if isSwitch {
            @display("p=396,422");
        }
        mac: <default("HiEthernetMac")> like IEtherMac {
            parameters:
                @display("p=435,502");
        }
    connections:
        upperLayerIn --> { @display("m=n"); } --> egressTC.in;
        if (isSwitch) {
            egressTC.out --> Hiqueue.in;
        //egressTC.out --> queue.in;

            Hiqueue.out --> Switchqueue.in;

            //egressTC.out --> queue.in;
            Switchqueue.out --> mac.upperLayerIn;
        }
        if (!isSwitch) {
            egressTC.out --> Hostqueue.in;
            Hostqueue.out --> mac.upperLayerIn;
        }

        mac.phys <--> { @display("m=s"); } <--> phys;

        mac.upperLayerOut --> ingressTC.in;
        ingressTC.out --> { @display("m=n"); } --> upperLayerOut;
}

