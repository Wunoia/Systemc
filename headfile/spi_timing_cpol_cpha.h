// CPOL/CPHA 时序生成
#ifndef SPI_TIMING_CPOL_CPHA_H
#define SPI_TIMING_CPOL_CPHA_H

#include "common.h"

SC_MODULE(spi_timing_cpol_cpha)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<sc_uint<32>>  sck_freq;       // SCK目标频率，来自分频模块
    sc_in<bool>         clk_en;         // 时钟总开关，来自寄存器使能
    sc_in<bool>         div_valid;      // 分频配置有效，来自分频模块
    sc_in<sc_uint<6>>   trans_width;    // 每帧bit数，来自分频/位宽模块
    sc_in<bool>         start_trig;     // 帧传输启动脉冲，来自FIFO模块
    sc_in<bool>         cpol;           // 时钟空闲电平，来自SPI模式配置
    sc_in<bool>         cpha;           // 采样边沿选择，来自SPI模式配置

    //===== 输出信号 =====
    sc_out<bool>        sck;            // SPI时钟物理信号，对外输出到从机
    sc_out<bool>        sample_en;      // 采样使能，通知移位寄存器抓取MISO
    sc_out<bool>        shift_en;       // 移位使能，通知移位寄存器移位

    SC_CTOR(spi_timing_cpol_cpha);
private:
    void timing_proc(); // 线程：按CPOL/CPHA事件驱动产生时序节拍
};

#endif