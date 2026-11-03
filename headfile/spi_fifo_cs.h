// 数据缓存，片选信号控制
#ifndef SPI_FIFO_CS_H
#define SPI_FIFO_CS_H

#include "common.h"

#define FIFO_DEPTH 8

SC_MODULE(spi_fifo_cs)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<sc_uint<32>>  tx_wdata;       // QEMU写入的发送数据，送入TX FIFO
    sc_in<bool>         tx_wen;         // TX FIFO写使能，脉冲有效时写入
    sc_in<bool>         rx_ren;         // RX FIFO读使能，QEMU读取时拉高
    sc_in<sc_uint<32>>  rx_parallel;    // 移位寄存器收满一帧的并行数据，压入RX FIFO
    sc_in<bool>         trans_done;     // 一帧传输完成脉冲，来自移位模块
    sc_in<sc_uint<8>>   cs_cfg;         // 片选配置：CS编号与极性，来自寄存器
    sc_in<bool>         start_bit;      // 软件启动位，来自CTRL寄存器

    //===== 输出信号 =====
    sc_out<sc_uint<32>> tx_parallel;    // 弹出TX FIFO数据，送给移位寄存器
    sc_out<bool>        start_trig;     // 启动传输触发脉冲，送给时序模块
    sc_out<sc_uint<32>> rx_rdata;       // 弹出RX FIFO数据，返回给QEMU
    sc_out<bool>        tx_fifo_empty;  // TX FIFO空标志，送状态模块
    sc_out<bool>        tx_fifo_full;   // TX FIFO满标志，送状态模块
    sc_out<bool>        rx_fifo_empty;  // RX FIFO空标志，送状态模块
    sc_out<bool>        rx_fifo_full;   // RX FIFO满标志，送状态模块
    sc_out<bool>        ovr_err;        // RX FIFO溢出错误标志，送状态模块
    sc_out<bool>        cs_n;           // 片选信号，低有效，对外输出到从机

    SC_CTOR(spi_fifo_cs);
private:
    void fifo_proc();   // THREAD：FIFO读写、溢出检测、产生start_trig
};

#endif