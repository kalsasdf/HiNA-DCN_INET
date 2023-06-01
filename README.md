
[HiNA-DCN inet项目.docx](https://github.com/kalsasdf/HiNA-DCN_INET/files/11594550/HiNA-DCN.inet.docx)

2023-6-1 RateLimitQueue优化，pullPacket()中添加scheduleAt自消息，可以做到更精确的限速；测速功能优化

2023-5-29 POSEIDON计算实时速率功能优化；HiUdpApp读取负载条件优化 

2023-5-27 修复所有TimerMsg在仿真结束后没能正确删除而弹出的警告;将非标准结构的包（如credit、ack等）的payload字段enableImplicitChunkSerailization置为true，使其可以在release模式下的仿真中点开而不需要额外的序列化文件；更正DT动态阈值计算方式

2023-5-12 新增HOMA；使用HiTag中的isLastPck标识优化了测量FCT的功能

2023-5-4 修复超时重传没考虑STOPPING状态的问题；优化共享缓存PFC的阈值判断

2023-5-1 新增POSEIDON；统一UdpCC模块代码格式；修复使用共享缓存时，队列长度数据为负的问题

2023-4-26 修复了SWIFT在release模式下的数组地址报错

2023-4-24 新增生成sendscript的python脚本以及app读取txt文件中负载信息的功能，删除了ECNQueue，全部使用共享缓存的REDPFCQueue

2023-4-17 TcpApp功能优化，补全了多socket接收的功能，修改了负载的指定方式和FCT的计算

2023-4-9 PFC代码优化

2023-4-3 新增SWIFT及代码优化

2023-3-17 优化TIMELY代码

2023-3-7 新增HPCC及代码优化

2023-2-24 新增TIMELY及代码优化
