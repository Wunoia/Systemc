#include "spi_status_gen.h"

spi_status_gen::spi_status_gen(sc_module_name name)
    : sc_module(name)
{
    SC_METHOD(status_proc);
    sensitive << rst_n << tx_fifo_empty << tx_fifo_full
              << rx_fifo_empty << rx_fifo_full << trans_done
              << ovr_err << busy << clear_events << status_read;
}

sc_uint<16> spi_status_gen::read_status()
{
    const sc_uint<16> snapshot = status_value();
    if (snapshot[SPI_SR_TC_BIT] || snapshot[SPI_SR_OVR_BIT])
    {
        tc_flag = false;
        ovr_flag = false;
        status_read.notify(SC_ZERO_TIME);
    }
    return snapshot;
}

sc_uint<16> spi_status_gen::status_value() const
{
    sc_uint<16> status = 0;
    if (rst_n.read())
    {
        status[SPI_SR_TXE_BIT] = tx_fifo_empty.read();
        status[SPI_SR_RXNE_BIT] = !rx_fifo_empty.read();
        status[SPI_SR_TC_BIT] = tc_flag;
        status[SPI_SR_OVR_BIT] = ovr_flag;
        status[SPI_SR_BUSY_BIT] = busy.read();
        status[SPI_SR_TX_FULL_BIT] = tx_fifo_full.read();
        status[SPI_SR_RX_FULL_BIT] = rx_fifo_full.read();
    }
    return status;
}

void spi_status_gen::status_proc()
{
    const bool active = rst_n.read();
    if (!active || clear_events.posedge())
    {
        tc_flag = false;
        ovr_flag = false;
    }

    if (active)
    {
        tc_flag = tc_flag || trans_done.posedge();
        // FIFO 将 ovr_err 保持到 RX 被读空，只锁存每次上升沿。
        ovr_flag = ovr_flag || ovr_err.posedge();
    }

    const sc_uint<16> status = status_value();
    status_reg.write(status);
    evt_txe.write(status[SPI_SR_TXE_BIT]);
    evt_rxne.write(status[SPI_SR_RXNE_BIT]);
    evt_tc.write(status[SPI_SR_TC_BIT]);
    evt_err.write(status[SPI_SR_OVR_BIT]);
}
