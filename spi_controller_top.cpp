#include "spi_controller_top.h"
#include <cstring>

SC_HAS_PROCESS(spi_controller_top);
spi_controller_top::spi_controller_top(sc_module_name name)
    : sc_module(name), tgt_socket("tgt_socket")
{
    tgt_socket.bind(*this);

    m_clk_div    = new spi_clk_div_width("m_clk_div");
    m_timing     = new spi_timing_cpol_cpha("m_timing");
    m_status     = new spi_status_gen("m_status");
    m_shift      = new spi_shift_reg("m_shift");
    m_fifo_cs    = new spi_fifo_cs("m_fifo_cs");
    m_irq_dma    = new spi_irq_dma("m_irq_dma");

    m_clk_div->rst_n(rst_n);
    m_clk_div->clk_sys_freq(clk_sys_freq);
    m_clk_div->div_cfg(div_cfg);
    m_clk_div->data_width_cfg(data_width_cfg);
    m_clk_div->sck_freq(sck_freq);
    m_clk_div->trans_width(trans_width);
    m_clk_div->div_valid(div_valid);

    m_timing->rst_n(rst_n);
    m_timing->sck_freq(sck_freq);
    m_timing->clk_en(clk_en);
    m_timing->div_valid(div_valid);
    m_timing->trans_width(trans_width);
    m_timing->start_trig(start_trig);
    m_timing->cpol(cpol);
    m_timing->cpha(cpha);
    m_timing->sck(sck);
    m_timing->sample_en(sample_en);
    m_timing->shift_en(shift_en);

    m_shift->rst_n(rst_n);
    m_shift->start_trig(start_trig);
    m_shift->sample_en(sample_en);
    m_shift->shift_en(shift_en);
    m_shift->trans_width(trans_width);
    m_shift->tx_parallel(tx_parallel);
    m_shift->miso_bit(miso);
    m_shift->mosi(mosi);
    m_shift->rx_parallel(rx_parallel);
    m_shift->trans_done(trans_done);
    m_shift->busy(busy);

    m_fifo_cs->rst_n(rst_n);
    m_fifo_cs->tx_wdata(tx_wdata);
    m_fifo_cs->tx_wen(tx_wen);
    m_fifo_cs->rx_ren(rx_ren);
    m_fifo_cs->rx_parallel(rx_parallel);
    m_fifo_cs->trans_done(trans_done);
    m_fifo_cs->cs_cfg(cs_cfg);
    m_fifo_cs->start_bit(start_bit);
    m_fifo_cs->tx_parallel(tx_parallel);
    m_fifo_cs->start_trig(start_trig);
    m_fifo_cs->rx_rdata(rx_rdata);
    m_fifo_cs->tx_fifo_empty(tx_fifo_empty);
    m_fifo_cs->tx_fifo_full(tx_fifo_full);
    m_fifo_cs->rx_fifo_empty(rx_fifo_empty);
    m_fifo_cs->rx_fifo_full(rx_fifo_full);
    m_fifo_cs->ovr_err(ovr_err);
    m_fifo_cs->cs_n(cs_n);

    m_status->rst_n(rst_n);
    m_status->tx_fifo_empty(tx_fifo_empty);
    m_status->tx_fifo_full(tx_fifo_full);
    m_status->rx_fifo_empty(rx_fifo_empty);
    m_status->rx_fifo_full(rx_fifo_full);
    m_status->trans_done(trans_done);
    m_status->ovr_err(ovr_err);
    m_status->busy(busy);
    m_status->clear_events(clear_events);
    m_status->status_reg(status_reg);
    m_status->evt_txe(evt_txe);
    m_status->evt_rxne(evt_rxne);
    m_status->evt_tc(evt_tc);
    m_status->evt_err(evt_err);

    m_irq_dma->rst_n(rst_n);
    m_irq_dma->evt_txe(evt_txe);
    m_irq_dma->evt_rxne(evt_rxne);
    m_irq_dma->evt_tc(evt_tc);
    m_irq_dma->evt_err(evt_err);
    m_irq_dma->int_en_reg(int_en_reg);
    m_irq_dma->dma_en_reg(dma_en_reg);
    m_irq_dma->irq(irq);
    m_irq_dma->dma_req(dma_req);

    
    SC_THREAD(clear_pulse_proc);
}

spi_controller_top::~spi_controller_top()
{
    delete m_clk_div;
    delete m_timing;
    delete m_status;
    delete m_shift;
    delete m_fifo_cs;
    delete m_irq_dma;
}

void spi_controller_top::clear_pulse_proc()
{
    while(true)
    {
        wait(tx_wen.value_changed_event()
             | rx_ren.value_changed_event()
             | clear_events.value_changed_event());

        wait(SC_ZERO_TIME);

        if(tx_wen.read())       tx_wen.write(false);
        if(rx_ren.read())       rx_ren.write(false);
        if(clear_events.read()) clear_events.write(false);
    }
}

void spi_controller_top::b_transport(tlm_generic_payload& trans, sc_time& delay)
{
    tlm_command cmd = trans.get_command();
    sc_uint<64> addr = trans.get_address();
    unsigned char* ptr = trans.get_data_ptr();
    unsigned int len = trans.get_data_length();
    unsigned char* be = trans.get_byte_enable_ptr();
    unsigned int be_len = trans.get_byte_enable_length();

    trans.set_response_status(TLM_OK_RESPONSE);
    if(len != 4 || (be != nullptr && be_len != 0))
    {
        trans.set_response_status(TLM_GENERIC_ERROR_RESPONSE);
        return;
    }

    unsigned int tmp_in = 0;
    memcpy(&tmp_in, ptr, 4);
    sc_uint<32> wdata = tmp_in;

    addr &= 0xFF;

    if(cmd == TLM_WRITE_COMMAND)
    {
        switch(addr)
        {
            case SPI_CR1:
                start_bit.write(wdata[0]);
                cpol.write(wdata[1]);
                cpha.write(wdata[2]);
                clk_en.write(wdata[3]);
                break;
            case SPI_BRR:
                div_cfg.write((sc_uint<8>)wdata);
                break;
            case SPI_WIDTH:
                data_width_cfg.write((sc_uint<6>)wdata);
                break;
            case SPI_TXDR:
                tx_wdata.write(wdata);
                tx_wen.write(true);
                break;
            case SPI_IER:
                int_en_reg.write((sc_uint<16>)wdata);
                break;
            case SPI_DMAER:
                dma_en_reg.write((sc_uint<16>)wdata);
                break;
            case SPI_CSCR:
                cs_cfg.write((sc_uint<8>)wdata);
                break;
            case SPI_RXDR:
            case SPI_SR:
                break;
            default:
                trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
                break;
        }
    }
    else if(cmd == TLM_READ_COMMAND)
    {
        sc_uint<32> rdata = 0;
        switch(addr)
        {
            case SPI_CR1:
                rdata[0] = start_bit.read();
                rdata[1] = cpol.read();
                rdata[2] = cpha.read();
                rdata[3] = clk_en.read();
                break;
            case SPI_BRR:
                rdata = div_cfg.read();
                break;
            case SPI_WIDTH:
                rdata = data_width_cfg.read();
                break;
            case SPI_TXDR:
                rdata = 0;
                break;
            case SPI_RXDR:
                rdata = rx_rdata.read();
                rx_ren.write(true);
                break;
            case SPI_SR:
                rdata = status_reg.read();
                clear_events.write(true);
                break;
            case SPI_IER:
                rdata = int_en_reg.read();
                break;
            case SPI_DMAER:
                rdata = dma_en_reg.read();
                break;
            case SPI_CSCR:
                rdata = cs_cfg.read();
                break;
            default:
                trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
                break;
        }
        unsigned int tmp_out = rdata.to_uint();
        memcpy(ptr, &tmp_out, 4);
    }

    delay = SC_ZERO_TIME;
}

tlm::tlm_sync_enum spi_controller_top::nb_transport_fw(
    tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
{
    trans.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
    return tlm::TLM_COMPLETED;
}

bool spi_controller_top::get_direct_mem_ptr(tlm_generic_payload& trans, tlm_dmi& dmi_data)
{
    return false;
}

unsigned int spi_controller_top::transport_dbg(tlm_generic_payload& trans)
{
    return 0;
}