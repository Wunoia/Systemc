// 对接 QEMU TLM Target Socket
#ifndef SPI_CONTROLLER_TOP_H
#define SPI_CONTROLLER_TOP_H

#include "common.h"
#include "spi_clk_div_width.h"
#include "spi_timing_cpol_cpha.h"
#include "spi_status_gen.h"
#include "spi_shift_reg.h"
#include "spi_fifo_cs.h"
#include "spi_irq_dma.h"

SC_MODULE(spi_controller_top), public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types>
{
    //===== TLM对外Socket，对接QEMU总线 =====
    tlm_target_socket<> tgt_socket;     // QEMU通过TLM读写各寄存器

    //===== 全局系统输入 =====
    sc_in<bool>         rst_n;          // 全局复位，低有效
    sc_in<sc_uint<32>>  clk_sys_freq;   // 系统主时钟频率(Hz)

    //===== SPI物理对外引脚 =====
    sc_out<bool>        sck;            // SPI时钟，输出到从机
    sc_out<bool>        mosi;           // 主发从收数据线，输出到从机
    sc_in<bool>         miso;           // 从发主收数据线，输入自外部从机
    sc_out<bool>        cs_n;           // 片选信号，低有效，输出到从机

    //===== 上报给QEMU的信号 =====
    sc_out<bool>        irq;            // 中断请求，上报QEMU中断控制器
    sc_out<bool>        dma_req;        // DMA请求，通知DMA模块

    SC_CTOR(spi_controller_top);
    ~spi_controller_top();

    virtual void b_transport(tlm_generic_payload& trans, sc_time& delay);
    virtual tlm::tlm_sync_enum nb_transport_fw(tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay);
    virtual bool get_direct_mem_ptr(tlm_generic_payload& trans, tlm_dmi& dmi_data);
    virtual unsigned int transport_dbg(tlm_generic_payload& trans);

private:
    // 清脉冲，避免 b_transport 里 wait
    void clear_pulse_proc();

    spi_clk_div_width*    m_clk_div;    // 分频与位宽子模块
    spi_timing_cpol_cpha* m_timing;     // 时序生成子模块
    spi_status_gen*       m_status;     // 状态生成子模块
    spi_shift_reg*        m_shift;      // 移位寄存器子模块
    spi_fifo_cs*          m_fifo_cs;    // FIFO与片选子模块
    spi_irq_dma*          m_irq_dma;    // 中断与DMA子模块

    // 内部连线信号（模块间握手与时序）
    sc_signal<sc_uint<32>> sck_freq;    // 分频→时序：SCK目标频率
    sc_signal<sc_uint<6>>  trans_width; // 分频→时序/移位：帧bit数

    sc_signal<bool> div_valid;          // 分频→时序：分频配置有效
    sc_signal<bool> clk_en;             // 寄存器→时序：时钟总开关

    sc_signal<bool> cpol;               // 寄存器→时序：时钟极性
    sc_signal<bool> cpha;               // 寄存器→时序：时钟相位
    sc_signal<bool> start_bit;          // 寄存器→FIFO：软件启动位
    sc_signal<bool> start_trig;         // FIFO→时序：启动传输脉冲

    sc_signal<bool> sample_en;          // 时序→移位：MISO采样使能
    sc_signal<bool> shift_en;           // 时序→移位：移位脉冲

    sc_signal<sc_uint<32>> tx_parallel; // FIFO→移位：待发送并行数据
    sc_signal<sc_uint<32>> rx_parallel; // 移位→FIFO：收满的并行数据

    sc_signal<sc_uint<32>> rx_rdata;    // FIFO→顶层：QEMU读取的接收数据

    sc_signal<bool> trans_done;         // 移位→FIFO/状态：帧完成脉冲
    sc_signal<bool> busy;               // 移位→状态：传输忙标志

    sc_signal<sc_uint<32>> tx_wdata;    // 顶层→FIFO：QEMU写入的发送数据
    sc_signal<bool> tx_wen;             // 顶层→FIFO：TX FIFO写使能
    sc_signal<bool> rx_ren;             // 顶层→FIFO：RX FIFO读使能
    sc_signal<sc_uint<8>> cs_cfg;       // 顶层→FIFO：片选配置

    sc_signal<sc_uint<6>> data_width_cfg; // 寄存器→分频：位宽配置
    sc_signal<sc_uint<8>> div_cfg;      // 寄存器→分频：分频系数

    sc_signal<bool> tx_fifo_empty;      // FIFO→状态：TX空
    sc_signal<bool> tx_fifo_full;       // FIFO→状态：TX满
    sc_signal<bool> rx_fifo_empty;      // FIFO→状态：RX空
    sc_signal<bool> rx_fifo_full;       // FIFO→状态：RX满
    sc_signal<bool> ovr_err;            // FIFO→状态：溢出错误

    sc_signal<sc_uint<16>> status_reg;  // 状态→顶层：打包状态寄存器
    sc_signal<bool> evt_txe;            // 状态→中断：TX空事件
    sc_signal<bool> evt_rxne;           // 状态→中断：RX非空事件
    sc_signal<bool> evt_tc;             // 状态→中断：传输完成事件
    sc_signal<bool> evt_err;            // 状态→中断：错误事件

    sc_signal<sc_uint<16>> int_en_reg;  // 寄存器→中断：中断使能掩码
    sc_signal<sc_uint<16>> dma_en_reg;  // 寄存器→中断：DMA使能掩码

    sc_signal<bool> clear_events;       // 寄存器→状态：清除事件标志
};

#endif