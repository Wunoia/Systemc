// 串并转换
#ifndef SPI_SHIFT_REG_H
#define SPI_SHIFT_REG_H

#include "common.h"

SC_MODULE(spi_shift_reg)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<bool>         start_trig;     // 启动传输触发脉冲，来自FIFO模块
    sc_in<bool>         sample_en;      // MISO采样使能节拍，来自时序模块
    sc_in<bool>         shift_en;       // 移位脉冲，来自时序模块
    sc_in<sc_uint<6>>   trans_width;    // 每帧传输bit数，来自分频/位宽模块
    sc_in<sc_uint<32>>  tx_parallel;    // 待发送并行数据，来自TX FIFO
    sc_in<bool>         miso_bit;       // 外部从机输入的MISO串行bit

    //===== 输出信号 =====
    sc_out<bool>        mosi;           // MOSI串行bit，发往从机
    sc_out<sc_uint<32>> rx_parallel;    // 收满一帧后的并行数据，送入RX FIFO
    sc_out<bool>        trans_done;     // 一帧收发完成脉冲，送状态模块
    sc_out<bool>        busy;           // 传输忙标志，送状态模块

    SC_CTOR(spi_shift_reg);
private:
    void shift_proc();  // THREAD：移位、采样、串并转换逻辑
};

#endif