// 状态标志生成
#ifndef SPI_STATUS_GEN_H
#define SPI_STATUS_GEN_H

#include "common.h"

SC_MODULE(spi_status_gen)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<bool>         tx_fifo_empty; // 输入：TX FIFO空标志
    sc_in<bool>         tx_fifo_full;  // 输入：TX FIFO满标志
    sc_in<bool>         rx_fifo_empty; // 输入：RX FIFO空标志
    sc_in<bool>         rx_fifo_full;  // 输入：RX FIFO满标志
    sc_in<bool>         trans_done;    // 输入：移位模块，一帧传输完成脉冲
    sc_in<bool>         ovr_err;       // 输入：FIFO模块，接收溢出错误
    sc_in<bool>         busy;           // 输入：移位模块，传输忙标志

    //===== 输出信号 =====
    sc_out<sc_uint<16>> status_reg;    // 输出：打包后的状态寄存器，供QEMU读取
    sc_out<bool>        evt_txe;        // 输出事件：发送缓冲区空
    sc_out<bool>        evt_rxne;       // 输出事件：接收非空（收到新数据）
    sc_out<bool>        evt_tc;         // 输出事件：帧传输完成
    sc_out<bool>        evt_err;        // 输出事件：错误（溢出）

    SC_CTOR(spi_status_gen);
private:
    void status_proc(); // METHOD：组合逻辑组装status、产生事件
};

#endif
