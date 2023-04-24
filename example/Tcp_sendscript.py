""" 定制化TCPApp流量.读入网络条件和要生成的负载特征,为每个发送端主机的TCP模块生成流量脚本 """
import random
import os

current_file_path = os.path.abspath(__file__)           # 当前文件所在的目录
load_results_path = os.path.dirname(current_file_path) + os.sep #+ "load_results" + os.sep  # 负载文件生成位置



def GenTCPload(netparams, load_pattern):
    '''根据网络条件、生成相应的负载模式'''
    if load_pattern == "incast":
        GenTCPincast(netparams)
    elif load_pattern == "bufferdrop":
        GenTCPBufDrop(netparams)
    else:
        GenTCPGeneralLoad(netparams)   

def GenTCPincast(netparams):
    """根据网络条件生成【incast】负载,
    incast负载的特征是多对一地同时(数百us~ms级别)发送流量"""
    linkspeed = float(netparams["linkspeed"]) * 1e9
    sender_num = int(netparams["sendhost"])
    receiver_num = int(netparams["receivehost"])
    starttime = round(float(netparams["starttime"]), 6) # 时间戳的计算保留6位小数
    closetime = round(float(netparams["closetime"]), 6)
    load = float(netparams["load"])
    
    # 生成周期性的incast流量
    incast_flowsize = 1000  # 单位B
    timestamp_list = []     # 存放TCPApp流量的时间戳
    # 根据负载比例，确定incast流量的生成间隔
    current_time = starttime 
    while current_time < closetime:
        timestamp_list.append(round(current_time, 6))    
        # current_time += (8 * incast_flowsize) / (linkspeed * load) + round(random.uniform(0, 0.001), 6)
        current_time += (8 * incast_flowsize) / (linkspeed * load) 
    file = open(load_results_path + str(sender_num) + " to " + str(receiver_num) + " incast_load" + str(load) + ".txt", "w+")
    for timestamp in timestamp_list:
        file.write(str(timestamp))
        file.write(" ")
        file.write(str(incast_flowsize))
        file.write(";")
    file.close()
    #load_pattern_str

def GenTCPBufDrop(netparams):
    print("GenTCPBufDrop!\n")

def GenTCPGeneralLoad(netparams):
    print("GenTCPGeneralLoad!\n")

def main():
    # 输入网络条件: 链路速率(Gbps)\仿真开始时刻(s)\仿真结束时刻(s)\发送主机节点个数\接收主机个数
    netparams = {
        "linkspeed": "10",
        "starttime": "0.4",  
        "closetime": "0.40001", 
        "sendhost": "10",
        "receivehost": "1",
        "load": "1"
    }
    # 输入负载特征
    load_pattern = "incast"         # 选择负载模式{"incast", "bufferdrop"}
    GenTCPload(netparams, load_pattern)
    print(load_results_path)
if __name__ == "__main__":
    main()

