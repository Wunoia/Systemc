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
    tlm_target_socket<> tgt_socket;

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

    SC_CTOR(spi_controller_top);
    ~spi_controller_top();

    virtual void b_transport(tlm_generic_payload& trans, sc_time& delay);
    virtual tlm::tlm_sync_enum nb_transport_fw(tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay);
    virtual bool get_direct_mem_ptr(tlm_generic_payload& trans, tlm_dmi& dmi_data);
    virtual unsigned int transport_dbg(tlm_generic_payload& trans);

private:
    // 清脉冲，避免 b_transport 里 wait
    void clear_pulse_proc();

    spi_clk_div_width*    m_clk_div;
    spi_timing_cpol_cpha* m_timing;
    spi_status_gen*       m_status;
    spi_shift_reg*        m_shift;
    spi_fifo_cs*          m_fifo_cs;
    spi_irq_dma*          m_irq_dma;

    sc_signal<sc_uint<32>> sck_freq;

    sc_signal<sc_uint<6>> trans_width;

    sc_signal<bool> div_valid; //
    sc_signal<bool> clk_en;

    sc_signal<bool> cpol;
    sc_signal<bool> cpha;
    sc_signal<bool> start_bit;
    sc_signal<bool> start_trig;
    sc_signal<bool> sample_en;
    sc_signal<bool> shift_en;

    sc_signal<sc_uint<32>> tx_parallel;
    sc_signal<sc_uint<32>> rx_parallel;

    sc_signal<sc_uint<32>> rx_rdata;

    sc_signal<bool> trans_done;
    sc_signal<bool> busy;

    sc_signal<sc_uint<32>> tx_wdata;
    sc_signal<bool> tx_wen;
    sc_signal<bool> rx_ren;
    sc_signal<sc_uint<8>> cs_cfg;

    sc_signal<sc_uint<6>> data_width_cfg;
    sc_signal<sc_uint<8>> div_cfg;

    sc_signal<bool> tx_fifo_empty;
    sc_signal<bool> tx_fifo_full;
    sc_signal<bool> rx_fifo_empty;
    sc_signal<bool> rx_fifo_full;
    sc_signal<bool> ovr_err;

    sc_signal<sc_uint<16>> status_reg;
    sc_signal<bool> evt_txe;
    sc_signal<bool> evt_rxne;
    sc_signal<bool> evt_tc;
    sc_signal<bool> evt_err;

    sc_signal<sc_uint<16>> int_en_reg;
    sc_signal<sc_uint<16>> dma_en_reg;

    sc_signal<bool> clear_events;

};

#endif
