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

package inet.HiNA.Ethernet.HiQueue.BFCQueue.BFCqueue;
import inet.queueing.base.PacketQueueBase;
import inet.queueing.contract.IPacketQueue;


simple BFCqueue extends PacketQueueBase like IPacketQueue
{
    parameters:
        //队列包容量；队列中字节容量
        int packetCapacity = default(-1); // maximum number of packets in the queue, no limit by default
        int dataCapacity @unit(b) = default(-1b); // maximum total length of packets in the queue, no limit by default
        string dropperClass = default("inet::queueing::PacketAtCollectionEndDropper"); // determines which packets are dropped when the queue is overloaded, packets are not dropped by default; the parameter must be the name of a C++ class which implements the IPacketDropperFunction C++ interface and is registered via Register_Class
        string comparatorClass = default(""); // determines the order of packets in the queue, insertion order by default; the parameter must be the name of a C++ class which implements the IPacketComparatorFunction C++ interface and is registered via Register_Class
        displayStringTextFormat = default("contains %p pk (%l) pushed %u\npulled %o removed %r dropped %d");
        //int priority = default(-1);
        //FOR BFC
        int numb = default(-1);
        bool useBfc = default(true);
        int XON @unit(B) = default(0B);
        int XOFF @unit(B) = default(0B);
        bool useEcn = default(false);
        int Kmax @unit(B) = default(0B);
        int Kmin @unit(B) = default(0B);
        double Pmax = default(0);
        int upstreamQueueID = default(-1);
        int sharedBuffer @unit(b)= default(16Mb);
        int headroom @unit(b) = default(1b);//sharedBuffer的最小值；
        double alpha = default(1);//queue队列的maxSize 为剩余sharedBuffer的alpha倍，一般为2的整数次幂，可正可负。
        @class(BFCqueue);
        @signal[packetPushStarted](type=inet::Packet);
        @signal[packetPushEnded](type=inet::Packet?);//A question mark after the class name means that the signal is allowed to emit nullptr pointers.
        @signal[packetPulled](type=inet::Packet);
        @signal[packetRemoved](type=inet::Packet);
        @signal[packetDropped](type=inet::Packet);
        
        @signal[bfcPaused](type=inet::Packet);
        @signal[bfcResume](type=inet::Packet);
                     
        //由于添加了共享缓存，对于length统计的两个指标要用cOut...
        // the statistical value is the number of packets in the queue
        //@statistic[queueLength](title="queue length"; source=warmup(atomic(constant0(packetPushEnded) + count(packetPushStarted) - count(packetPulled) - count(packetRemoved) - count(packetDropped))); record=last,max,timeavg,vector; unit=pk; interpolationmode=sample-hold; autoWarmupFilter=false);
        // the statistical value is the total bit length of all packets in the queue
        //@statistic[queueBitLength](title="queue bit length"; source=warmup(atomic(constant0(packetPushEnded) + sum(packetLength(packetPushStarted)) - sum(packetLength(packetPulled)) - sum(packetLength(packetRemoved)) - sum(packetLength(packetDropped)))); record=last,max,timeavg,vector; unit=b; interpolationmode=sample-hold; autoWarmupFilter=false);
        // the statistical value is the queueing time of packets
        @statistic[queueingTime](title="queueing times"; source=queueingTime(packetPulled); record=histogram,vector; unit=s; interpolationmode=none);
        // the statistical value is the incoming packet
        @statistic[incomingPackets](title="incoming packets"; source=packetPushStarted; record=count; unit=pk);
        // the statistical value is the length of the incoming packet
        @statistic[incomingPacketLengths](title="incoming packet lengths"; source=packetLength(packetPushStarted); record=sum,histogram,vector; unit=b; interpolationmode=none);
        // the statistical value is the data rate of the incoming packets
        @statistic[incomingDataRate](title="incoming datarate"; source=throughput(packetPushStarted); record=vector; unit=bps; interpolationmode=linear);
        // the statistical value is the outgoing packet
        @statistic[outgoingPackets](title="outgoing packets"; source=packetPulled; record=count; unit=pk);
        // the statistical value is the length of the outgoing packet
        @statistic[outgoingPacketLengths](title="outgoing packet lengths"; source=packetLength(packetPulled); record=sum,histogram,vector; unit=b; interpolationmode=none);
        // the statistical value is the data rate of the outgoing packets
        @statistic[outgoingDataRate](title="outgoing datarate"; source=throughput(packetPulled); record=vector; unit=bps; interpolationmode=linear);
        // the statistical value is the packet that is dropped due to queue overflow
        @statistic[droppedPacketsQueueOverflow](title="dropped packets: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count; unit=pk; interpolationmode=none);
        // the statistical value is the length of the packet that is dropped due to queue overflow
        @statistic[droppedPacketLengthsQueueOverflow](title="dropped packet lengths: queue overflow"; source=packetLength(packetDropReasonIsQueueOverflow(packetDropped)); record=sum,vector; unit=b; interpolationmode=none);
        // the statistical value is the flow specific queueing time of packets
        @statistic[flowQueueingTime](title="flow queueing times"; source=queueingTime(demuxFlow(packetPulled)); record=histogram,vector; unit=s; interpolationmode=none);
        // the statistical value is the flow specific data rate of the incoming packets
        @statistic[flowIncomingDataRate](title="flow specific incoming data rate"; source=throughput(flowPacketLength(demuxFlow(packetPushStarted))); record=vector; unit=bps; interpolationmode=linear);
        // the statistical value is the flow specific data rate of the outgoing packets
        @statistic[flowOutgoingDataRate](title="flow specific outgoing data rate"; source=throughput(flowPacketLength(demuxFlow(packetPulled))); record=vector; unit=bps; interpolationmode=linear);
        // the statistical value is the pfcPaused packet
        @statistic[sendBfcPausedPck](title="BFCqueue send Bfc Paused Packet"; source=bfcPaused; record=count; unit=pk);
        // the statistical value is the pfcResume packet
        @statistic[sendBfcResumePck](title="BFCqueue send Bfc Resume Packet"; source=bfcResume; record=count; unit=pk);
        
        @defaultStatistic(queueLength:vector);
}
