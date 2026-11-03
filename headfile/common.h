// common.h
#ifndef COMMON_H
#define COMMON_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
using namespace tlm;

// 寄存器偏移宏定义
#define SPI_CR1     0x00
#define SPI_BRR     0x04
#define SPI_WIDTH   0x08
#define SPI_TXDR    0x0C
#define SPI_RXDR    0x10
#define SPI_SR      0x14
#define SPI_IER     0x18
#define SPI_DMAER   0x1C
#define SPI_CSCR    0x20

#endif
