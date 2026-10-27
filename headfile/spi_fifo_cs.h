// 数据缓存，片选信号控制
#ifndef SPI_FIFO_CS_H
#define SPI_FIFO_CS_H

#include "common.h"

SC_MODULE(spi_fifo_cs)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<sc_uint<32>>  tx_wdata;       // 输入：QEMU写TXDR，待写入TX FIFO数据
    sc_in<bool>         tx_wen;          // 输入：TX FIFO写使能
    sc_in<bool>         rx_ren;          // 输入：RX FIFO读使能
    sc_in<sc_uint<32>>  rx_parallel;     // 输入：移位模块收到的并行数据，压入RX FIFO
    sc_in<bool>         trans_done;      // 输入：移位模块，帧传输完成
    sc_in<sc_uint<8>>   cs_cfg;          // 输入：CS配置（片选编号+极性）
    sc_in<bool>         start_bit;       // 输入：CTRL寄存器启动位，软件发起传输

    //===== 输出信号 =====
    sc_out<sc_uint<32>> tx_parallel;     // 输出：弹出TX FIFO数据给移位寄存器
    sc_out<bool>        start_trig;      // 输出：启动传输触发脉冲，送给时序模块
    sc_out<bool>        tx_fifo_empty;   // 输出：TX FIFO空标志
    sc_out<bool>        tx_fifo_full;    // 输出：TX FIFO满标志
    sc_out<bool>        rx_fifo_empty;   // 输出：RX FIFO空标志
    sc_out<bool>        rx_fifo_full;    // 输出：RX FIFO满标志
    sc_out<bool>        ovr_err;         // 输出：RX FIFO溢出错误标志
    sc_out<bool>        cs_n;            // 输出：片选信号，低有效，对外

    SC_CTOR(spi_fifo_cs);
private:
    void fifo_proc();   // THREAD：FIFO读写、溢出检测、产生start_trig
    void cs_proc();     // METHOD：译码cs_cfg，产生cs_n电平
};

#endif
