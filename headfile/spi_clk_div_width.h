// 时钟分频与波特率生成，传输位宽配置
#ifndef SPI_CLK_DIV_WIDTH_H
#define SPI_CLK_DIV_WIDTH_H

#include "common.h"

SC_MODULE(spi_clk_div_width)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<sc_uint<32>>  clk_sys_freq;   // 输入：系统主时钟频率(Hz)
    sc_in<sc_uint<8>>   div_cfg;        // 输入：分频配置寄存器值（BAUD）
    sc_in<sc_uint<4>>   data_width_cfg; // 输入：位宽配置，支持4/8/16/32bit

    //===== 输出信号 =====
    sc_out<sc_uint<32>> sck_freq;       // 输出：计算得到的SCK目标频率，送给时序模块
    sc_out<sc_uint<4>>  trans_width;    // 输出：当前帧传输bit数，送给移位寄存器
    sc_out<bool>        clk_en;         // 输出：时钟总使能，1允许产生SCK节拍

    SC_CTOR(spi_clk_div_width);
private:
    void calc_proc();   // 线程：计算分频、输出频率与位宽
};

#endif
