// 中断通知，DMA请求触发
#ifndef SPI_IRQ_DMA_H
#define SPI_IRQ_DMA_H

#include "common.h"

SC_MODULE(spi_irq_dma)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<bool>         evt_txe;        // TX FIFO空事件，来自状态模块
    sc_in<bool>         evt_rxne;       // RX FIFO非空事件，来自状态模块
    sc_in<bool>         evt_tc;         // 帧传输完成事件，来自状态模块
    sc_in<bool>         evt_err;        // 错误事件（溢出），来自状态模块
    sc_in<sc_uint<16>>  int_en_reg;     // 中断使能掩码，来自IER寄存器
    sc_in<sc_uint<16>>  dma_en_reg;     // DMA使能掩码，来自DMAER寄存器

    //===== 输出信号 =====
    sc_out<bool>        irq;            // 最终中断请求，上报QEMU中断控制器
    sc_out<bool>        dma_req;        // DMA请求信号，通知DMA模块取/放数据

    SC_CTOR(spi_irq_dma);
private:
    void comb_proc();   // METHOD：事件 & 掩码组合，生成irq/dma_req
};

#endif