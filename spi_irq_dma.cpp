#include "spi_irq_dma.h"

spi_irq_dma::spi_irq_dma(sc_module_name name)
    : sc_module(name)
{
    SC_METHOD(comb_proc);
    sensitive << rst_n << evt_txe << evt_rxne << evt_tc << evt_err
              << int_en_reg << dma_en_reg;
}

void spi_irq_dma::comb_proc()
{
    const bool active = rst_n.read();
    sc_uint<16> events = 0;
    events[SPI_SR_TXE_BIT] = evt_txe.read();
    events[SPI_SR_RXNE_BIT] = evt_rxne.read();
    events[SPI_SR_TC_BIT] = evt_tc.read();
    events[SPI_SR_OVR_BIT] = evt_err.read();

    const sc_uint<16> dma_pending = events & dma_en_reg.read();
    const bool tx_request = active && dma_pending[SPI_SR_TXE_BIT];
    const bool rx_request = active && dma_pending[SPI_SR_RXNE_BIT];
    irq.write(active && (events.to_uint() & int_en_reg.read().to_uint()) != 0);
    dma_req.write(tx_request || rx_request);
    if (dma_tx_req.size() != 0)
        dma_tx_req->write(tx_request);
    if (dma_rx_req.size() != 0)
        dma_rx_req->write(rx_request);
}
