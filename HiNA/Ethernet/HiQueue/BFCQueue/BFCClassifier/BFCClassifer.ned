

package inet.HiNA.Ethernet.HiQueue.BFCQueue.BFCClassifier;
//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


import inet.queueing.base.PacketClassifierBase;
import inet.queueing.contract.IPacketClassifier;


simple BFCClassifier extends PacketClassifierBase like IPacketClassifier
{
    parameters:
        string Nums = default(""); // space separated dscp values of the gates, both names (e.g. AF11, EF) and numbers (0x0A,0b101110) can be used
        string priorities = default("");
        //string displayStringTextFormat = default("rate: %b\nsent: %s, rcvd: %r\nqueue: %q, drop: %d");
        @display("i=block/classifier");
        @class(BFCClassifier);
        @signal[packetClassified](type=inet::Packet);
        
        @statistic[classifiedPackets](title="classifier packets"; source=packetClassified; record=count; unit=pk);

        //@signal[pkClass](type=long);
        //@statistic[pkClass](title="packet class"; source=pkClass; record=vector; interpolationmode=none);
    gates:
        output defaultOut;
        output creditOut;
}
