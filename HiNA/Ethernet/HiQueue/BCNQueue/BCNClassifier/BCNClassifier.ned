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

package inet.HiNA.Ethernet.HiQueue.BCNQueue.BCNClassifier;
//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

import inet.queueing.base.PacketClassifierBase;
import inet.queueing.contract.IPacketClassifier;

//
// This classifier pushes packets into the first non-full among its connected
// packet consumers.
//
simple BCNClassifier extends PacketClassifierBase like IPacketClassifier
{
    parameters:
        string Nums = default(""); // space separated dscp values of the gates, both names (e.g. AF11, EF) and numbers (0x0A,0b101110) can be used
        string priorities = default("");
        //string displayStringTextFormat = default("rate: %b\nsent: %s, rcvd: %r\nqueue: %q, drop: %d");
        @display("i=block/classifier");
        @class(BCNClassifier);
        @signal[packetClassified](type=inet::Packet);
        
        //计数也可以写一个
        @statistic[classifiedPackets](title="classifier packets"; source=packetClassified; record=count; unit=pk);

        //@signal[pkClass](type=long);
        //@statistic[pkClass](title="packet class"; source=pkClass; record=vector; interpolationmode=none);
    gates:
        output defaultOut;
        output creditOut;
}
