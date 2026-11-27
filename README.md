# Systemc
Model Mounting Verification Based on SystemC

### 运行测试用例
在MSYS2 UCRT64终端中依次运行：
```
cd 源码对应目录
mkdir build && cd build
camke .. -G Ninja
ninja
./main.exe
```
即可运行测试用例main.cpp
正确输出如下：
```
        SystemC 3.0.2-Accellera --- Sep  1 2026 20:27:59
        Copyright (c) 1996-2025 by all Contributors,
        ALL RIGHTS RESERVED
start tester
number of tests:1
复位与读写测试
 复位期间 SR = 0x0000
 复位期间 irq/dam_req = 0
 复位后 SR = 0x0001;TX FIFO 为空
 BRR 读回一致
 WIDTH 读回一致
 IER 读回一致
 DMAER 读回一致
 CSCR 读回一致
 CR1 读回一致
number of tests:2
读写长度非4字节的数据测试
 测试: 读写操作不允许长度为非4字节的数据
number of tests:3
未知寄存器访问测试
 测试: 读写操作不允许访问未知的寄存器
number of tests:4
单帧回环,FIFO,CS状态测试
写入TXDR后TXE = 0;
CS = 0
传输完成
收到数据后 RXNE=1
回环接收数据 = 0x5A
RX 读空后 RXNE=0
start 清零后 CS 自动释放(高)
number of tests:5
cpol = 1,cpha = 1时传输测试
CS = 0
传输完成
CPOL=1,CPHA=1时 SCK = 1
回环接收数据 = 0xC3A5
start 清零后 CS 自动释放(高)
number of tests:6
TX FIFO 测试
4 帧写入后 TX FIFO 未满
4 帧传输完成
回环接收数据 = 0x11
回环接收数据 = 0x22
回环接收数据 = 0x33
回环接收数据 = 0x44
number of tests:7
RX FIFO 溢出测试
写满 8 帧后 TXFULL=1
8 帧收完后 RXFULL=1
// 注：此处为测试溢出状态，因此报错是正常的
Warning: spi_fifo_cs: RX FIFO overflow, frame dropped
In file: C:/msys64/home/Asus/Systemc-self/spi_fifo_cs.cpp:242
In process: tests.top.m_fifo_cs.fifo_proc @ 151586 ns
第 9 帧触发 OVR=1
溢出后回环接收数据 = 0x0
溢出后回环接收数据 = 0x1
溢出后回环接收数据 = 0x2
溢出后回环接收数据 = 0x3
溢出后回环接收数据 = 0x4
溢出后回环接收数据 = 0x5
溢出后回环接收数据 = 0x6
溢出后回环接收数据 = 0x7
RX 读空后 OVR 自动清除
number of tests:8
中断 IER(TC) 与 DMA 请求 DMAER(TXE)
TX FIFO 为空时产生 DMA 请求
关闭 DMAER 后请求撤销
TC 事件置位 irq
TC 状态位置位
清除事件后 irq 撤销
TC 用例接收数据正确
number of tests:9
片选控制 CSCR (CS_OFF / CS_POL)
CS_OFF=1 时传输中片选保持无效
CS_OFF 用例传输完成
CS_POL=1 时传输中片选为高
CS_POL 用例传输完成
CS_POL=1 时空闲片选为低
number of tests:a
4bit 位宽帧传输
4bit 用例传输完成
4bit 回环接收 = 0xB


```
