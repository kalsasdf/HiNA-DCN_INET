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

package inet.HiNA.CC.POSEIDON;

import inet.HiNA.CC.UdpCC;

simple POSEIDON like UdpCC
{
    parameters:
        bool activate = default(true);
        double linkspeed @unit(bps) = default (1Gbps);
        double stopTime @unit(s) = default(10s);
 		int max_pck_size = default(1470);
 		double maxInterval @unit(s)= default(20us);
 		double baseRTT @unit(s) = default(1us);
 		double PARA_P = default(40);
        double PARA_K = default(2);
        double m = default(0.25);
        double min_md = default(0.5);
      
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
        @signal[Usignal](type=double);
        @statistic[U](title="HPCC utilization"; source=Usignal; record=vector); 

    gates:
        input upperIn @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
}