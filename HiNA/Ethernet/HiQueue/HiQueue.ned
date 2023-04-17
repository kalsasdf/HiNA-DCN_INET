
package inet.HiNA.Ethernet.HiQueue;

import inet.HiNA.Ethernet.HiQueue.REDPFCQueue.REDPFCQueue;
import inet.HiNA.Ethernet.HiQueue.RateLimitQueue.RateLimitQueue;
import inet.HiNA.Ethernet.HiQueue.HiClassifier.HiClassifier;
import inet.HiNA.Ethernet.HiQueue.HiScheduler.HiPriorityScheduler;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.ITrafficConditioner;
import inet.HiNA.Ethernet.HiQueue.HiScheduler.WrrScheduler;

module HiQueue
{
    parameters:
        bool usePfc = default(false);
        *.usePfc = this.usePfc;
        bool useEcn = default(false);
        *.useEcn = this.useEcn;
        @display("bgb=692.05493,771.147");
    gates:
        input in;
        output out;
    submodules:
        classifier: HiClassifier {
            priorities = "0 1 2 3 4 5 6 7 ";
            @display("p=100,330");
        }
        beQueue: <default("ECNQueue")> like IPacketQueue {
            dataCapacity = default(3200000b);
            @display("p=249.55,616.9;is=s");
        }
        creditQueue: RateLimitQueue {
            dataCapacity = default(5376b); // 8 credits, for expresspass
            @display("p=249.55,51.15;is=s");
        }
        Priority_0: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 0;
            @display("p=250,120;is=s");
        }
        Priority_1: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 1;
            @display("p=250,180;is=s");
        }
        Priority_2: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 2;
            @display("p=250,240;is=s");
        }
        Priority_3: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 3;
            @display("p=250,300;is=s");
        }
        Priority_4: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 4;
            @display("p=250,360;is=s");
        }
        Priority_5: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 5;
            @display("p=250,420;is=s");
        }
        Priority_6: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 6;
            @display("p=250,480;is=s");
        }
        Priority_7: <default("REDPFCQueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            priority = 7;
            @display("p=250,540;is=s");
        }
        PriorityScheduler: HiPriorityScheduler {
            @display("p=458.80002,328.6");
        }
        WrrScheduler: WrrScheduler {
            weights = "1 1 1";
            @display("p=590.55,328.6");
        }
    connections:
        in --> classifier.in;
        classifier.creditOut --> creditQueue.in;
        classifier.out++ --> Priority_0.in;
        classifier.out++ --> Priority_1.in;
        classifier.out++ --> Priority_2.in;
        classifier.out++ --> Priority_3.in;
        classifier.out++ --> Priority_4.in;
        classifier.out++ --> Priority_5.in;
        classifier.out++ --> Priority_6.in;
        classifier.out++ --> Priority_7.in;
        classifier.defaultOut --> beQueue.in;

        Priority_0.out --> PriorityScheduler.in++;
        Priority_1.out --> PriorityScheduler.in++;
        Priority_2.out --> PriorityScheduler.in++;
        Priority_3.out --> PriorityScheduler.in++;
        Priority_4.out --> PriorityScheduler.in++;
        Priority_5.out --> PriorityScheduler.in++;
        Priority_6.out --> PriorityScheduler.in++;
        Priority_7.out --> PriorityScheduler.in++;
        beQueue.out --> WrrScheduler.in++;
        creditQueue.out --> WrrScheduler.in++;
        PriorityScheduler.out --> WrrScheduler.in++;
        WrrScheduler.out --> out;
}

