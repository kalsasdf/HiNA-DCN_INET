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

package inet.HiNA.Ethernet.HiQueue.BFCQueue;


import inet.HiNA.Ethernet.HiQueue.BFCQueue.BFCqueue.BFCqueue;
import inet.HiNA.Ethernet.HiQueue.BFCQueue.BFCClassifier.BFCClassifier;
import inet.HiNA.Ethernet.HiQueue.BFCQueue.BFCScheduler.BFCScheduler;
import inet.HiNA.Ethernet.HiQueue.RateLimitQueue.RateLimitQueue;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.ITrafficConditioner;
import inet.HiNA.Ethernet.HiQueue.HiScheduler.WrrScheduler;
import inet.queueing.queue.DropTailQueue;

module BFCQueue
{
    parameters:
        bool useBfc = default(false);
        *.useBfc = this.useBfc;
        bool useEcn = default(false);
        *.useEcn = this.useEcn;
        @display("bgb=743,818");
        
    gates:
        input in;
        output out;
    submodules:
        BfcClassifier: BFCClassifier {
            Nums = "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 ";
            @display("p=80.76923,417.69232");
        }
       // beQueue: <default("BFCqueue")> like IPacketQueue {
       beQueue: DropTailQueue {
          //  dataCapacity = default(160000b);
           // numb = 99;
            @display("p=287,758;is=s");
        }
        creditQueue: RateLimitQueue {
            dataCapacity = default(5376b); // 8 credits, for expresspass
            @display("p=287,78;is=s");
        }
        Num_0: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 0;
            @display("p=287,98;is=s");
        }
        Num_1: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 1;
            @display("p=287,118;is=s");
        }
        Num_2: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 2;
            @display("p=287,138;is=s");
        }
        Num_3: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 3;
            @display("p=287,158;is=s");
        }
        Num_4: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 4;
            @display("p=287,178;is=s");
        }
        Num_5: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 5;
            @display("p=287,198;is=s");
        }
        Num_6: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 6;
            @display("p=287,218;is=s");
        }
        Num_7: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 7;
            @display("p=287,238;is=s");
        }
        Num_8: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 7;
            @display("p=287,258;is=s");
        }
        Num_9: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 9;
            @display("p=287,278;is=s");
        }
        Num_10: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 10;
            @display("p=287,298;is=s");
        }
        Num_11: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 11;
            @display("p=287,318;is=s");
        }
        Num_12: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 12;
            @display("p=287,338;is=s");
        }
        Num_13: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 13;
            @display("p=287,358;is=s");
        }
        Num_14: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 14;
            @display("p=287,378;is=s");
        }
        Num_15: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 15;
            @display("p=287,398;is=s");
        }
        Num_16: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 16;
            @display("p=287,418;is=s");
        }
        Num_17: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 17;
            @display("p=287,438;is=s");
        }
        Num_18: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 18;
            @display("p=287,458;is=s");
        }
        Num_19: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 19;
            @display("p=287,478;is=s");
        }
        Num_20: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 20;
            @display("p=287,498;is=s");
        }
        Num_21: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 21;
            @display("p=287,518;is=s");
        }
        Num_22: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 22;
            @display("p=287,538;is=s");
        }
        Num_23: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 23;
            @display("p=287,558;is=s");
        }
        Num_24: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 24;
            @display("p=287,578;is=s");
        }
        Num_25: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 25;
            @display("p=287,598;is=s");
        }
        Num_26: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 26;
            @display("p=287,618;is=s");
        }
        Num_27: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 27;
            @display("p=287,638;is=s");
        }
        Num_28: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 28;
            @display("p=287,658;is=s");
        }
        Num_29: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 29;
            @display("p=287,678;is=s");
        }
        Num_30: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 30;
            @display("p=287,698;is=s");
        }
        Num_31: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 31;
            @display("p=287,718;is=s");
        }
        Num_32: <default("BFCqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 32;
            @display("p=287,738;is=s");
        }

        BfcScheduler: BFCScheduler {
            weights01 = "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1" ;
            @display("p=536,418");
        }
        BfcWrrScheduler: WrrScheduler {
            weights = "1 1 1";
            @display("p=651,418");
        }
    connections:
        in --> BfcClassifier.in;
        BfcClassifier.creditOut --> creditQueue.in;
        BfcClassifier.out++ --> Num_0.in;
        BfcClassifier.out++ --> Num_1.in;
        BfcClassifier.out++ --> Num_2.in;
        BfcClassifier.out++ --> Num_3.in;
        BfcClassifier.out++ --> Num_4.in;
        BfcClassifier.out++ --> Num_5.in;
        BfcClassifier.out++ --> Num_6.in;
        BfcClassifier.out++ --> Num_7.in;
        BfcClassifier.out++ --> Num_8.in;
        BfcClassifier.out++ --> Num_9.in;
        BfcClassifier.out++ --> Num_10.in;

        BfcClassifier.out++ --> Num_11.in;
        BfcClassifier.out++ --> Num_12.in;
        BfcClassifier.out++ --> Num_13.in;
        BfcClassifier.out++ --> Num_14.in;
        BfcClassifier.out++ --> Num_15.in;
        BfcClassifier.out++ --> Num_16.in;
        BfcClassifier.out++ --> Num_17.in;
        BfcClassifier.out++ --> Num_18.in;
        BfcClassifier.out++ --> Num_19.in;
        BfcClassifier.out++ --> Num_20.in;

        BfcClassifier.out++ --> Num_21.in;
        BfcClassifier.out++ --> Num_22.in;
        BfcClassifier.out++ --> Num_23.in;
        BfcClassifier.out++ --> Num_24.in;
        BfcClassifier.out++ --> Num_25.in;
        BfcClassifier.out++ --> Num_26.in;
        BfcClassifier.out++ --> Num_27.in;
        BfcClassifier.out++ --> Num_28.in;
        BfcClassifier.out++ --> Num_29.in;
        BfcClassifier.out++ --> Num_30.in;

        BfcClassifier.out++ --> Num_31.in;
        BfcClassifier.out++ --> Num_32.in;

        BfcClassifier.defaultOut --> beQueue.in;

        Num_0.out --> BfcScheduler.in++;
        Num_1.out --> BfcScheduler.in++;
        Num_2.out --> BfcScheduler.in++;
        Num_3.out --> BfcScheduler.in++;
        Num_4.out --> BfcScheduler.in++;
        Num_5.out --> BfcScheduler.in++;
        Num_6.out --> BfcScheduler.in++;
        Num_7.out --> BfcScheduler.in++;
        Num_8.out --> BfcScheduler.in++;
        Num_9.out --> BfcScheduler.in++;
        Num_10.out --> BfcScheduler.in++;

        Num_11.out --> BfcScheduler.in++;
        Num_12.out --> BfcScheduler.in++;
        Num_13.out --> BfcScheduler.in++;
        Num_14.out --> BfcScheduler.in++;
        Num_15.out --> BfcScheduler.in++;
        Num_16.out --> BfcScheduler.in++;
        Num_17.out --> BfcScheduler.in++;
        Num_18.out --> BfcScheduler.in++;
        Num_19.out --> BfcScheduler.in++;
        Num_20.out --> BfcScheduler.in++;

        Num_21.out --> BfcScheduler.in++;
        Num_22.out --> BfcScheduler.in++;
        Num_23.out --> BfcScheduler.in++;
        Num_24.out --> BfcScheduler.in++;
        Num_25.out --> BfcScheduler.in++;
        Num_26.out --> BfcScheduler.in++;
        Num_27.out --> BfcScheduler.in++;
        Num_28.out --> BfcScheduler.in++;
        Num_29.out --> BfcScheduler.in++;
        Num_30.out --> BfcScheduler.in++;

        Num_31.out --> BfcScheduler.in++;
        Num_32.out --> BfcScheduler.in++;


        beQueue.out --> BfcWrrScheduler.in++;
        creditQueue.out --> BfcWrrScheduler.in++;
        BfcScheduler.out --> BfcWrrScheduler.in++;
        BfcWrrScheduler.out --> out;
}

