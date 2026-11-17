// 状态标志生成
#ifndef SPI_STATUS_GEN_H
#define SPI_STATUS_GEN_H

#include "common.h"

SC_MODULE(spi_status_gen)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<bool>         tx_fifo_empty;  // TX FIFO空标志，来自FIFO模块
    sc_in<bool>         tx_fifo_full;   // TX FIFO满标志，来自FIFO模块
    sc_in<bool>         rx_fifo_empty;  // RX FIFO空标志，来自FIFO模块
    sc_in<bool>         rx_fifo_full;   // RX FIFO满标志，来自FIFO模块
    sc_in<bool>         trans_done;     // 一帧传输完成脉冲，来自移位模块
    sc_in<bool>         ovr_err;        // RX FIFO溢出错误，来自FIFO模块
    sc_in<bool>         busy;           // 传输忙标志，来自移位模块
    sc_in<bool>         clear_events;   // 独立使用时的事件清除输入

    //===== 输出信号 =====
    sc_out<sc_uint<16>> status_reg;     // 打包后的状态寄存器，供QEMU读取
    sc_out<bool>        evt_txe;        // 事件：发送缓冲区空
    sc_out<bool>        evt_rxne;       // 事件：接收非空（收到新数据）
    sc_out<bool>        evt_tc;         // 事件：帧传输完成
    sc_out<bool>        evt_err;        // 事件：错误（溢出）

    SC_CTOR(spi_status_gen);
    sc_uint<16> read_status();           // 读取快照并清除其中的锁存事件
private:
    void status_proc(); // 组装状态并锁存完成和溢出事件
    sc_uint<16> status_value() const;
    sc_event status_read;
    bool tc_flag = false;
    bool ovr_flag = false;
};

#endif
