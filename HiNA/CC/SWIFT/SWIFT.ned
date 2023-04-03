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

package inet.HiNA.CC.SWIFT;

import inet.HiNA.CC.UdpCC;
//
// TODO auto-generated type
//
simple SWIFT like UdpCC
{
    parameters:
        bool activate = default(true);
        double linkspeed @unit(bps) = default (10Gbps);
        double stopTime @unit(s) = default(10s);
        int max_pck_size = default(1472);
        double baseRTT @unit(s) = default(1s);
        double ai = default(1);
        double beta = default(0.8);
        double max_mdf = default(0.5);
 		double hop_scaler @unit(s)= default(0.0000003s);
 		int hops = default(2);
        double fs_min_cwnd = default(0.1);
        double fs_max_cwnd = default(100);

        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
    gates:
        input upperIn @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
}
