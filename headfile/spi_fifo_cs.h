// 数据缓存，片选信号控制
#ifndef SPI_FIFO_CS_H
#define SPI_FIFO_CS_H

#include "common.h"

// 单个 FIFO 的深度，单位：帧（每帧最多 32bit）
#define FIFO_DEPTH 8

//===== SPI_CSCR(0x20) 片选控制寄存器位定义 =====
//  bit[0]    CS_OFF  1 = 强制关闭片选（cs_n 恒为无效电平）
//                    0 = 由传输过程自动控制（复位默认值）
//  bit[1]    CS_POL  片选有效电平：0 = 低有效（复位默认），1 = 高有效
//  bit[3:2]          保留，写 0
//  bit[7:4]  CS_ID   从机编号 0~15；本模型只有一根 cs_n，仅作记录，
//                    为将来扩展多路片选译码预留
#define CSCR_CS_OFF_POS     0
#define CSCR_CS_POL_POS     1
#define CSCR_CS_ID_POS      4

//===== 内部状态机 =====
//  ST_IDLE   : 片选无效，等待软件置 start 位
//  ST_ACTIVE : 序列已开启（片选有效），但当前没有帧在收发，等待数据
//  ST_FRAME  : 一帧正在收发，等 trans_done 到来
//=====

SC_MODULE(spi_fifo_cs)
{
    //===== 输入信号 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效，来自顶层
    sc_in<sc_uint<32>>  tx_wdata;       // QEMU写入的发送数据，送入TX FIFO
    sc_in<bool>         tx_wen;         // TX FIFO写使能，脉冲有效时写入
    sc_in<bool>         rx_ren;         // RX FIFO读使能，QEMU读取时拉高
    sc_in<sc_uint<32>>  rx_parallel;    // 移位寄存器收满一帧的并行数据，压入RX FIFO
    sc_in<bool>         trans_done;     // 一帧传输完成脉冲，来自移位模块
    sc_in<sc_uint<8>>   cs_cfg;         // 片选配置：CS编号与极性，来自寄存器
    sc_in<bool>         start_bit;      // 软件启动位，来自CTRL寄存器

    //===== 输出信号 =====
    sc_out<sc_uint<32>> tx_parallel;    // 弹出TX FIFO数据，送给移位寄存器
    sc_out<bool>        start_trig;     // 启动传输触发脉冲，送给时序模块
    sc_out<sc_uint<32>> rx_rdata;       // 弹出RX FIFO数据，返回给QEMU
    sc_out<bool>        tx_fifo_empty;  // TX FIFO空标志，送状态模块
    sc_out<bool>        tx_fifo_full;   // TX FIFO满标志，送状态模块
    sc_out<bool>        rx_fifo_empty;  // RX FIFO空标志，送状态模块
    sc_out<bool>        rx_fifo_full;   // RX FIFO满标志，送状态模块
    sc_out<bool>        ovr_err;        // RX FIFO溢出错误标志，送状态模块
    sc_out<bool>        cs_n;           // 片选信号，低有效，对外输出到从机

    SC_CTOR(spi_fifo_cs);
private:
    void fifo_proc();   // THREAD：FIFO读写、溢出检测、产生start_trig
    void pulse_proc();  // THREAD：把内部事件整形为单 delta 宽的 start_trig 脉冲

    //===== 内部工具 =====
    void reset_state();                 // 复位指针、空满标志与状态机
    void handle_tx_write();             // 处理一次 TXDR 写入
    void handle_rx_read();              // 处理一次 RXDR 读取
    void handle_start_bit();            // 处理 start 位跳变
    void handle_frame_done();           // 处理一帧收发完成
    void start_frame();                 // 弹出一帧数据并启动一次收发
    void apply_cs();                    // 按状态机与 cs_cfg 驱动 cs_n

    bool push_tx(const sc_uint<32>& data);
    bool pop_tx(sc_uint<32>& data);
    bool push_rx(const sc_uint<32>& data);
    bool pop_rx(sc_uint<32>& data);
    void update_flags();                // 刷新 empty/full 标志输出
    void update_rx_head();              // 刷新 rx_rdata 为 RX 队首数据

    //===== 内部状态 =====
    enum state_t { ST_IDLE, ST_ACTIVE, ST_FRAME };

    state_t     m_state;                // 片选/传输状态机
    bool        m_cs_on;                // 片选当前是否处于“有效”状态
    bool        m_ovr;                  // RX 溢出标志（粘滞到 RX 被读空）
    sc_event    m_start_evt;            // 请求发一次 start_trig 脉冲

    // TX FIFO（循环队列）
    sc_uint<32> m_tx_mem[FIFO_DEPTH];
    unsigned int m_tx_wp;               // 写指针
    unsigned int m_tx_rp;               // 读指针
    unsigned int m_tx_cnt;              // 当前帧数

    // RX FIFO（循环队列）
    sc_uint<32> m_rx_mem[FIFO_DEPTH];
    unsigned int m_rx_wp;               // 写指针
    unsigned int m_rx_rp;               // 读指针
    unsigned int m_rx_cnt;              // 当前帧数
};

#endif
