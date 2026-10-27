// 中断通知，DMA请求触发
#ifndef SPI_IRQ_DMA_H
#define SPI_IRQ_DMA_H

#include "common.h"

SC_MODULE(spi_irq_dma)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<bool>         evt_txe;        // 输入：状态模块，TX空事件
    sc_in<bool>         evt_rxne;       // 输入：状态模块，RX非空事件
    sc_in<bool>         evt_tc;         // 输入：状态模块，传输完成事件
    sc_in<bool>         evt_err;        // 输入：状态模块，错误事件
    sc_in<sc_uint<16>>  int_en_reg;     // 输入：中断使能掩码寄存器
    sc_in<sc_uint<16>>  dma_en_reg;     // 输入：DMA使能掩码寄存器

    //===== 输出信号 =====
    sc_out<bool>        irq;            // 输出：最终中断请求，上报QEMU中断控制器
    sc_out<bool>        dma_req;        // 输出：DMA请求信号，通知DMA模块

    SC_CTOR(spi_irq_dma);
private:
    void comb_proc();   // METHOD：事件 & 掩码组合，生成irq/dma_req
};

#endif
