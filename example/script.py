'''
FilePath: /python_workplace/omnetpp-script/script.py
Description: Customize the traffic pattern for the host.
Version: 1.0
Author: Lu Yuan
Date: 2023-04-18 14:36:03
LastEditTime: 2023-05-30 11:24:44
'''

import random
from random import randint, randint
import os

# ../
current_file_path = os.path.abspath(__file__)   
# ../script/
scripts_path = os.path.dirname(current_file_path) + os.sep #+ "script" + os.sep 

def main():
    # network parameters
    NetParams = {
        "linkspeed": "10",
        "delay": "2"
    }
    # The user sets the traffic transmission mode for each host.
    HostTrafficTrace =  {
        # Key:Values in this dictionary are: 
        # (str) <HostAppName>:  (list)[Traffic pattern series] 
        # "Hosts1app0": [
        #       # Each list is any combination of the two modes.
        #       ["startTime", "stopTime", "mode", "workload"],
        #       ["simtime_t txBytes;"]      
        #  ]
        "Hosts1app0":[
            ["0", "0.05", "CacheFollower", "0.4"],
            ["0.1", "0.12", "WebServer", "0.7"],
            ["0.12", "0.15", "WebSearch", "0.4"],
            ["0.1566 5000;"],
            ["0.16", "0.17", "HPCep", "0.1"],
            ["0.17", "0.19", "HPCcg", "0.6"],
            ["0.19", "0.21", "HPCft", "0.5"],
            ["0.21", "0.25", "HPCmg", "0.9"],
            ["0.22 1500;"],
            ["0.23", "20", "DataMining", "0.2"],
        ],
        "Hosts2app0":[
            ["0", "0.05", "CacheFollower", "0.4"],
            ["0.1", "0.12", "WebServer", "0.7"],
            ["0.12", "0.15", "WebSearch", "0.4"],
            ["0.1566 5000;"],
            ["0.16", "0.17", "HPCep", "0.1"],
            ["0.17", "0.19", "HPCcg", "0.6"],
            ["0.19", "0.21", "HPCft", "0.5"],
            ["0.21", "0.25", "HPCmg", "0.9"],
            ["0.22 1500;"],
            ["0.23", "20", "DataMining", "0.2"],
        ]
    }
    GenerateTraces(NetParams, HostTrafficTrace)
    

def updateFlow(seed, mode):
    if mode == "CacheFollower":
        # 50% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 29% 1mb~, average 701kb
        if seed <= 0.01:
            return 70
        elif seed <= 0.015:
            return randint(0, 150 - 71) + 71
        elif seed == 0.04:
            return 150
        elif seed <= 0.08:
            return randint(0, 300 - 151) + 151
        elif seed < 0.1:
            return randint(0, 350 - 301) + 301
        elif seed <= 0.19:
            return 350
        elif seed <= 0.2:
            return randint(0, 450 - 351) + 351
        elif seed <= 0.28:
            return randint(0, 500 - 451) + 451
        elif seed <= 0.3:
            return randint(0, 600 - 501) + 501
        elif seed <= 0.35:
            return randint(0, 700 - 601) + 601
        elif seed <= 0.4:
            return randint(0, 1100 - 701) + 701
        elif seed <= 0.42:
            return randint(0, 2000 - 1101) + 1101
        elif seed <= 0.48:
            return randint(0, 10000- 2001) + 2001
        elif seed <= 0.5:
            return randint(0, 30000 - 10001) + 10001
        elif seed <= 0.52:
            return randint(0, 100000 - 30001) + 30001
        elif seed <= 0.6:
            return randint(0, 200000 - 100001) + 100001
        elif seed <= 0.68:
            return randint(0, 400000 - 200001) + 200001
        elif seed <= 0.7:
            return randint(0, 600000 - 400001) + 400001
        elif seed <= 0.701:
            return randint(0, 15000 - 6001) * 100 + 600001
        elif seed <= 0.8:
            return randint(0, 20000 - 15001) * 100 + 1500001
        elif seed <= 0.9:
            return randint(0, 24000 - 20001) * 100 + 2000001
        elif seed <= 1:
            return randint(0, 30000 - 24001) * 100 + 2400001
    elif mode == "DataMining":
        # 78% 0~10kb, 5% 10~100kb, 8% 100kb~1mb, 9% 1mb~, average 7410kb
        if seed <= 0.8:
            return randint(0, 10000 - 101) + 101
        elif seed <= 0.8346:
            return randint(0, 15252 - 1001) * 10 + 10001
        elif seed <= 0.9:
            return randint(0, 39054 - 15253) * 10 + 152523
        elif seed <= 0.953846:
            return randint(0, 32235 - 3906) * 100 + 390542
        elif seed <= 0.99:
            return randint(0, 10000 - 323) * 10000 + 3223543
        elif seed <= 1:
            return randint(0, 10000 - 1001) * 100000 + 100000001
    elif mode == "WebServer":
        # 63% 0~10kb, 18% 10~100kb, 19% 100kb~1mb, 0% 1mb~, average 64kb
        if seed <= 0.12:
            return randint(0, 300 - 151) + 151
        elif seed <= 0.2:
            return 300
        elif seed <= 0.3:
            return randint(0, 1000 - 601) + 601
        elif seed <= 0.4:
            return randint(0, 2000 - 1001) + 1001
        elif seed <= 0.5:
            return randint(0, 3100 - 2001) + 2001
        elif seed <= 0.6:
            return randint(0, 6000 - 3101) + 3101
        elif seed <= 0.71:
            return randint(0, 20000 - 6001) + 6001          
        elif seed <= 0.8:
            return randint(0, 6000 - 2001) * 10 + 20001
        elif seed <= 0.82:
            return randint(0, 15000 - 6001) * 10 + 60001
        elif seed <= 0.9:
            return randint(0, 30000-15001) * 10 + 150001
        elif seed <= 1:
            return randint(0, 50000 - 30001) * 10 + 300001
    elif mode == "WebSearch":
        # 49% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 20% 1mb~ big14000kb), average 1600kb
        if seed <= 0.15:
            return 9000
        elif seed <= 0.2:
            return randint(0, 18582-9001) + 9001
        elif seed <= 0.3:
            return randint(0, 28140-18583) + 18583
        elif seed <= 0.4:
            return randint(0, 38913-28141) + 28141
        elif seed <= 0.53:
            return randint(0, 7747-3892) * 10 + 38914
        elif seed <= 0.6:
            return randint(0, 20000-7747) * 10 + 77469
        elif seed <= 0.7:
            return randint(0, 10000-2001) * 100 + 200001
        elif seed <= 0.8:
            return randint(0, 20000-10001) * 100 + 1000001
        elif seed <= 0.9:
            return randint(0, 50000-20001) * 100 + 2000001
        elif seed <= 0.97:
            return randint(0, 10000-5001) * 1000 + 5000001
        elif seed <= 1:
            return randint(0, 30000-10001) * 1000 + 10000001
    elif mode == "HPCep":
        if seed <= 0.4436:
            return 48
        elif seed <= 0.7265:
            return 56
        elif seed <= 1:
            return 128
    elif mode == "HPCcg":
        if seed <= 0.6316:
            return 48
        elif seed <= 0.6345:
            return 128
        elif seed <= 0.8172:
            return 3544
        elif seed <= 1:
            return 3552
    elif mode == "HPCft":
        if seed <= 0.0226:
            return 48
        elif seed <= 0.0234:
            return 56
        elif seed <= 0.0242:
            return 64
        elif seed <= 0.025:
            return 80
        elif seed <= 0.0258:
            return 112
        elif seed <= 0.9115:
            return 128
        elif seed <= 0.9123:
            return 176
        elif seed <= 0.9131:
            return 304
        elif seed <= 0.921:
            return 308
        elif seed <= 0.9218:
            return 560
        elif seed <= 0.9889:
            return 1072
        elif seed <= 0.9697:
            return 2096
        elif seed <= 0.9905:
            return 4144
        elif seed <= 1:
            return 16432
    elif mode == "HPCmg":
        if seed<=0.0978:
            return 48
        elif seed<=0.4065:
            return 56
        elif seed<=0.6638:
            return 64
        elif seed<=0.6747:
            return 72
        elif seed<=0.6954:
            return 80
        elif seed<=0.6980:
            return 96
        elif seed<=0.7180:
            return 112
        elif seed<=0.7316:
            return 120
        elif seed<=0.7445:
            return 128
        elif seed<=0.7664:
            return 144
        elif seed<=0.7873:
            return 176
        elif seed<=0.7900:
            return 192
        elif seed<=0.8109:
            return 304
        elif seed<=0.8302:
            return 336
        elif seed<=0.8495:
            return 368
        elif seed<=0.8688:
            return 848
        elif seed<=0.8881:
            return 1072
        elif seed<=0.9074:
            return 1200
        elif seed<=0.9267:
            return 2640
        elif seed<=0.9511:
            return 4144
        elif seed<=0.9756:
            return 4400
        elif seed<=1:
            return 9296
    else:
        print("Unknown traffic mode!\n")
def generate(NetParams, traffic_pattern):
    # traffic_pattern properties
    startTime = round(float(traffic_pattern[0]), 8)
    stopTime = round(float(traffic_pattern[1]), 8)
    mode = str(traffic_pattern[2])
    workLoad = float(traffic_pattern[3])
    linkspeed = float(NetParams["linkspeed"]) * 1e9
    # generate traffic
    currentTime = startTime    
    tStampList = []
    txBytesList = []
    while currentTime < stopTime:
        seed = random.randint(0, 1)
        FlowLength = updateFlow(seed, mode)
        tStampList.append(round(currentTime, 8))  
        currentTime += (8 * FlowLength) / (linkspeed * workLoad)
        txBytesList.append(FlowLength)
    ans = ""
    for index in range(0, len(tStampList)):
        ans += str(tStampList[index])
        ans += " "
        ans += str(txBytesList[index])
        ans += ";"
    print("  %s: %.2fs-%.2fs avg-flowsize = %f KB, workload = %f Gbps" % (mode, startTime, stopTime, round(sum(txBytesList)/len(txBytesList)/1000, 2), 8 * sum(txBytesList) / (stopTime-startTime)/ 1e9))
    return ans

def GenerateTrafficPattern(NetParams, traffic_pattern):
    # @traffic_pattern_list:  A list of custom traffic patterns.  
    if ";" in traffic_pattern[0]:
        print("  "+traffic_pattern[0])
        return traffic_pattern[0]
    else:
        return generate(NetParams, traffic_pattern)

def GenerateTraces(NetParams, HostTrafficTrace):
    # @HostTrafficTrace:  Host-to-traffic pattern mapping
    for hostName in HostTrafficTrace:
        file = open(scripts_path + hostName + "Script.txt","w+");
        print(hostName + ': ')
        for traffic_pattern in HostTrafficTrace[hostName]:
            str = GenerateTrafficPattern(NetParams, traffic_pattern)
            file.write(str)
        file.write(";")
        file.close()



if __name__ == "__main__":
    main()