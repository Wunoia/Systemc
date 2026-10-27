// 串并转换
#ifndef SPI_SHIFT_REG_H
#define SPI_SHIFT_REG_H

#include "common.h"

SC_MODULE(spi_shift_reg)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<bool>         sample_en;      // 输入：时序模块，MISO采样使能
    sc_in<bool>         shift_en;       // 输入：时序模块，移位脉冲
    sc_in<sc_uint<4>>   trans_width;     // 输入：本次帧传输bit宽度
    sc_in<sc_uint<32>>  tx_parallel;     // 输入：FIFO送来的待发送并行数据
    sc_in<bool>         miso_bit;        // 输入：外部从机MISO串行bit

    //===== 输出信号 =====
    sc_out<bool>        mosi;            // 输出：MOSI串行bit，发往从机
    sc_out<sc_uint<32>> rx_parallel;     // 输出：收满一帧后的并行数据，送入RX FIFO
    sc_out<bool>        trans_done;      // 输出：一帧收发完成脉冲，送状态模块
    sc_out<bool>        busy;            // 输出：忙标志，传输中拉高

    SC_CTOR(spi_shift_reg);
private:
    void shift_proc();  // THREAD：移位、采样、串并转换逻辑
};

#endif
