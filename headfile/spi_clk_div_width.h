// 时钟分频与波特率生成，传输位宽配置
#ifndef SPI_CLK_DIV_WIDTH_H
#define SPI_CLK_DIV_WIDTH_H

#include "common.h"

SC_MODULE(spi_clk_div_width)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<sc_uint<32>>  clk_sys_freq;   // 系统主时钟频率(Hz)，来自顶层
    sc_in<sc_uint<8>>   div_cfg;        // 分频值；SCK = clk_sys_freq / div_cfg
    sc_in<sc_uint<6>>   data_width_cfg; // 位宽配置，支持4/8/16/32bit，来自寄存器

    //===== 输出信号 =====
    sc_out<sc_uint<32>> sck_freq;       // 计算得到的SCK目标频率，送给时序模块
    sc_out<sc_uint<6>>  trans_width;    // 当前帧传输bit数，送给移位寄存器
    sc_out<bool>        div_valid;      // 配置有效，1表示SCK频率和位宽有效

    SC_CTOR(spi_clk_div_width);
private:
    void calc_proc();   // 组合逻辑：计算分频、输出频率与位宽
};

#endif
