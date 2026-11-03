// common.h
#ifndef COMMON_H
#define COMMON_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
using namespace tlm;

// 寄存器偏移宏定义
#define SPI_CR1     0x00    // 控制寄存器1：使能、CPOL/CPHA、启动位等
#define SPI_BRR     0x04    // 波特率寄存器：SCK分频系数
#define SPI_WIDTH   0x08    // 数据位宽寄存器：配置每帧4/8/16/32 bit
#define SPI_TXDR    0x0C    // 发送数据寄存器：QEMU写入待发数据
#define SPI_RXDR    0x10    // 接收数据寄存器：QEMU读取收到的数据
#define SPI_SR      0x14    // 状态寄存器：FIFO空满、忙、错误等状态
#define SPI_IER     0x18    // 中断使能寄存器：各事件中断屏蔽
#define SPI_DMAER   0x1C    // DMA使能寄存器：DMA请求屏蔽
#define SPI_CSCR    0x20    // 片选控制寄存器：CS编号与极性

#endif
