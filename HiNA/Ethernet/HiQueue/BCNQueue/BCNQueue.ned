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

package inet.HiNA.Ethernet.HiQueue.BCNQueue;



import inet.HiNA.Ethernet.HiQueue.BCNQueue.BCNqueue.BCNqueue;
import inet.HiNA.Ethernet.HiQueue.BCNQueue.BCNClassifier.BCNClassifier;
import inet.HiNA.Ethernet.HiQueue.BCNQueue.BCNScheduler.BCNScheduler;
import inet.HiNA.Ethernet.HiQueue.RateLimitQueue.RateLimitQueue;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.ITrafficConditioner;
import inet.HiNA.Ethernet.HiQueue.HiScheduler.WrrScheduler;
import inet.queueing.queue.DropTailQueue;


module BCNQueue
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
        BcnClassifier: BCNClassifier {
            Nums = "0 1 2 3 4 5 6 7";
            @display("p=83.84615,447.69232");
        }
        
        beQueue: DropTailQueue {
           
            @display("p=286.9231,710.7693;is=s");
        }
        creditQueue: RateLimitQueue {
            dataCapacity = default(5376b); // 8 credits, for expresspass
            @display("p=286.9231,206.15385;is=s");
        }
        Num_0: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 0;
            @display("p=286.9231,256.9231;is=s");
        }
        Num_1: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 1;
            @display("p=286.9231,309.23077;is=s");
        }
        Num_2: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 2;
            @display("p=286.9231,355.38464;is=s");
        }
        Num_3: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 3;
            @display("p=286.9231,411.53848;is=s");
        }
        Num_4: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 4;
            @display("p=286.9231,456.9231;is=s");
        }
        Num_5: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 5;
            @display("p=286.9231,521.53845;is=s");
        }
        Num_6: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 6;
            @display("p=286.9231,581.5385;is=s");
        }
        Num_7: <default("BCNqueue")> like IPacketQueue {
            dataCapacity = default(160000b);
            numb = 7;
            @display("p=286.9231,642.30774;is=s");
        }
        BcnScheduler: BCNScheduler {
            weights02 = "1 1 1 1 1 1 1 1";
            @display("p=507.69232,456.9231");
        }
        BcnWrrScheduler: WrrScheduler {
			weights = "1 1 1";
            @display("p=641.5385,456.9231");
        }
    connections:
        in --> BcnClassifier.in;
        BcnClassifier.creditOut --> creditQueue.in;
        BcnClassifier.out++ --> Num_0.in;
        BcnClassifier.out++ --> Num_1.in;
        BcnClassifier.out++ --> Num_2.in;
        BcnClassifier.out++ --> Num_3.in;
        BcnClassifier.out++ --> Num_4.in;
        BcnClassifier.out++ --> Num_5.in;
        BcnClassifier.out++ --> Num_6.in;
        BcnClassifier.out++ --> Num_7.in;


        BcnClassifier.defaultOut --> beQueue.in;

        Num_0.out --> BcnScheduler.in++;
        Num_1.out --> BcnScheduler.in++;
        Num_2.out --> BcnScheduler.in++;
        Num_3.out --> BcnScheduler.in++;
        Num_4.out --> BcnScheduler.in++;
        Num_5.out --> BcnScheduler.in++;
        Num_6.out --> BcnScheduler.in++;
        Num_7.out --> BcnScheduler.in++;

        beQueue.out --> BcnWrrScheduler.in++;
        creditQueue.out --> BcnWrrScheduler.in++;
        BcnScheduler.out --> BcnWrrScheduler.in++;
        BcnWrrScheduler.out --> out;
}

