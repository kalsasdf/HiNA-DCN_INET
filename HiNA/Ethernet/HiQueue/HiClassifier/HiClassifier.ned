//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.HiNA.Ethernet.HiQueue.HiClassifier;

import inet.queueing.base.PacketClassifierBase;
import inet.queueing.contract.IPacketClassifier;

//
// This classifier pushes packets into the first non-full among its connected
// packet consumers.
//
simple HiClassifier extends PacketClassifierBase like IPacketClassifier
{
    parameters:
        string priorities = default(""); // space separated dscp values of the gates, both names (e.g. AF11, EF) and numbers (0x0A,0b101110) can be used
        @display("i=block/classifier");
        @class(HiClassifier);

        //@signal[pkClass](type=long);
        //@statistic[pkClass](title="packet class"; source=pkClass; record=vector; interpolationmode=none);
    gates:
        output defaultOut;
        output creditOut;
}
