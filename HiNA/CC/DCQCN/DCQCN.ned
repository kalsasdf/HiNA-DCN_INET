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

package inet.HiNA.CC.DCQCN;

import inet.HiNA.CC.UdpCC;
// TODO documentation
//

simple DCQCN like UdpCC
{
    parameters:
        bool activate = default(true);
        double linkspeed @unit(bps) = default (1Gbps);
        double stopTime @unit(s) = default(10s);
 		double min_cnp_interval @unit(s) = default(50us);
 		double initialrate=default(1);
 		double AlphaTimer_th @unit(s) = default(55us);
 		double RateTimer_th @unit(s) = default(55us);
 		int ByteCounter_th @unit(B) = default(10MiB);
 		double Rai @unit(bps) = default(4e7bps);
 		double Rhai @unit(bps) = default(4e8bps);
 		int frSteps_th = default(5);
 		double gamma = default(1/256);
 		int max_pck_size = default(1472);
        
        // If unschedPrioResolutionMode is set to EXPLICIT,
        // then this string defines the priority cutoff points of unsched
        // bytes for the remaining message sizes. Example would be "100 1500 9000"
        //string explicitUnschedPrioCutoff = default("");
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
    gates:
        input upperIn @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
}


