// CPOL/CPHA 时序生成
#ifndef SPI_TIMING_CPOL_CPHA_H
#define SPI_TIMING_CPOL_CPHA_H

#include "common.h"

SC_MODULE(spi_timing_cpol_cpha)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<sc_uint<32>>  sck_freq;       // 输入：来自分频模块，SCK目标频率
    sc_in<bool>         clk_en;         // 输入：时钟总开关
    sc_in<bool>         start_trig;     // 输入：来自FIFO，帧传输启动触发脉冲
    sc_in<bool>         cpol;           // 输入：SPI模式配置，时钟空闲电平
    sc_in<bool>         cpha;           // 输入：SPI模式配置，采样边沿选择

    //===== 输出信号 =====
    sc_out<bool>        sck;            // 输出：SPI时钟物理信号，对外输出
    sc_out<bool>        sample_en;      // 输出：采样使能，通知移位寄存器抓取MISO
    sc_out<bool>        shift_en;       // 输出：移位使能，通知移位寄存器移位

    SC_CTOR(spi_timing_cpol_cpha);
private:
    void timing_proc(); // 线程：按CPOL/CPHA事件驱动产生时序节拍
};

#endif
