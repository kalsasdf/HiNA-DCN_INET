//
// Copyright (C) 2012 OpenSim Ltd
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
package inet.HiNA.CC;

moduleinterface UdpCC
{
    parameters:
        @display("i=block/ifcard");
    gates:
        input upperIn @labels(UdpControlInfo/down) @messageKinds(inet::UdpCommandCode);// higher layer
        output upperOut @labels(UdpControlInfo/up) @messageKinds(inet::UdpStatusInd);
        output lowerOut @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
}
