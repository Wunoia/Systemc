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

SC_MODULE(spi_controller_top)
{
    //===== TLM对外Socket，对接QEMU总线 =====
    tlm_target_socket<32> tgt_socket;

    //===== 全局系统输入 =====
    sc_in<bool>         rst_n;
    sc_in<sc_uint<32>>  clk_sys_freq;

    //===== SPI物理对外引脚 =====
    sc_out<bool>        sck;
    sc_out<bool>        mosi;
    sc_in<bool>         miso;
    sc_out<bool>        cs_n;

    //===== 上报给QEMU的信号 =====
    sc_out<bool>        irq;
    sc_out<bool>        dma_req;

    //===== 内部互联信号（模块之间连线） =====
    sc_signal<sc_uint<32>> sck_freq;
    sc_signal<sc_uint<4>>  trans_width;
    sc_signal<bool>        clk_en;

    sc_signal<bool> sample_en;
    sc_signal<bool> shift_en;
    sc_signal<bool> start_trig;

    sc_signal<sc_uint<32>> rx_parallel;
    sc_signal<bool> trans_done;
    sc_signal<bool> busy;

    sc_signal<sc_uint<32>> tx_parallel;
    sc_signal<bool> tx_fifo_empty;
    sc_signal<bool> tx_fifo_full;
    sc_signal<bool> rx_fifo_empty;
    sc_signal<bool> rx_fifo_full;
    sc_signal<bool> ovr_err;

    sc_signal<bool> evt_txe;
    sc_signal<bool> evt_rxne;
    sc_signal<bool> evt_tc;
    sc_signal<bool> evt_err;
    sc_signal<sc_uint<16>> status_reg;

    sc_signal<sc_uint<16>> int_en_reg;
    sc_signal<sc_uint<16>> dma_en_reg;

    // 寄存器配置分发信号（TLM解析后输出）
    sc_signal<sc_uint<8>>  div_cfg;
    sc_signal<bool>        cpol;
    sc_signal<bool>        cpha;
    sc_signal<sc_uint<8>>  cs_cfg;
    sc_signal<bool>        start_bit;
    sc_signal<sc_uint<4>>  data_width_cfg;
    sc_signal<sc_uint<32>> tx_wdata;
    sc_signal<bool>        tx_wen;
    sc_signal<bool>        rx_ren;

    //===== 子模块实例指针 =====
    spi_clk_div_width*      m_clk_div;
    spi_timing_cpol_cpha*   m_timing;
    spi_status_gen*         m_status;
    spi_shift_reg*          m_shift;
    spi_fifo_cs*            m_fifo_cs;
    spi_irq_dma*            m_irq_dma;

    SC_CTOR(spi_controller_top);
private:
    // TLM transport回调，处理QEMU的寄存器读写payload
    void b_transport(tlm_generic_payload& trans, sc_time& delay);
};

#endif
