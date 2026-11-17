#include "spi_controller_top.h"

namespace
{
constexpr unsigned bit(SpiStatusBit position)
{
    return 1u << position;
}

constexpr unsigned txe_bit = bit(SPI_SR_TXE_BIT);
constexpr unsigned rxne_bit = bit(SPI_SR_RXNE_BIT);
constexpr unsigned tc_bit = bit(SPI_SR_TC_BIT);
constexpr unsigned ovr_bit = bit(SPI_SR_OVR_BIT);

SC_MODULE(status_irq_dma_test)
{
    spi_status_gen status{"status"};
    spi_irq_dma notifications{"notifications"};
    sc_signal<bool> rst_n, tx_empty, tx_full, rx_empty, rx_full;
    sc_signal<bool> trans_done, overflow, busy, clear;
    sc_signal<bool> txe, rxne, tc, err, irq, dma;
    sc_signal<sc_uint<16>> sr, ier, dmaer;

    SC_CTOR(status_irq_dma_test)
    {
        status.rst_n(rst_n);
        status.tx_fifo_empty(tx_empty);
        status.tx_fifo_full(tx_full);
        status.rx_fifo_empty(rx_empty);
        status.rx_fifo_full(rx_full);
        status.trans_done(trans_done);
        status.ovr_err(overflow);
        status.busy(busy);
        status.clear_events(clear);
        status.status_reg(sr);
        status.evt_txe(txe);
        status.evt_rxne(rxne);
        status.evt_tc(tc);
        status.evt_err(err);

        notifications.rst_n(rst_n);
        notifications.evt_txe(txe);
        notifications.evt_rxne(rxne);
        notifications.evt_tc(tc);
        notifications.evt_err(err);
        notifications.int_en_reg(ier);
        notifications.dma_en_reg(dmaer);
        notifications.irq(irq);
        notifications.dma_req(dma);
        SC_THREAD(run);
    }

    void run()
    {
        tx_empty.write(true);
        rx_empty.write(true);
        ier.write(tc_bit);
        dmaer.write(txe_bit);
        wait(1, SC_NS);
        sc_assert(sr.read() == 0 && !irq.read() && !dma.read());

        rst_n.write(true);
        wait(1, SC_NS);
        sc_assert(sr.read() == txe_bit && !irq.read() && dma.read());

        tx_empty.write(false);
        tx_full.write(true);
        rx_empty.write(false);
        rx_full.write(true);
        busy.write(true);
        trans_done.write(true);
        overflow.write(true);
        dmaer.write(rxne_bit);
        wait(1, SC_NS);
        const unsigned fifo_status = rxne_bit | bit(SPI_SR_BUSY_BIT)
                                   | bit(SPI_SR_TX_FULL_BIT) | bit(SPI_SR_RX_FULL_BIT);
        sc_assert(sr.read() == (fifo_status | tc_bit | ovr_bit));
        sc_assert(irq.read() && dma.read());

        trans_done.write(false);
        ier.write(ovr_bit);
        dmaer.write(tc_bit | ovr_bit | (1u << 15));
        wait(1, SC_NS);
        sc_assert(sr.read() == (fifo_status | tc_bit | ovr_bit));
        sc_assert(irq.read() && !dma.read());

        clear.write(true);
        trans_done.write(true);
        wait(1, SC_NS);
        sc_assert(sr.read() == (fifo_status | tc_bit));
        sc_assert(!irq.read() && !dma.read());

        clear.write(false);
        trans_done.write(false);
        wait(1, SC_NS);
        sc_assert(sr.read() == (fifo_status | tc_bit));

        overflow.write(false);
        wait(1, SC_NS);
        overflow.write(true);
        wait(1, SC_NS);
        sc_assert(irq.read());

        rst_n.write(false);
        wait(1, SC_NS);
        sc_assert(sr.read() == 0 && !irq.read() && !dma.read());
    }
};

SC_MODULE(spi_register_test)
{
    spi_controller_top top{"spi"};
    tlm_utils::simple_initiator_socket<spi_register_test> socket{"socket"};
    sc_signal<bool> rst_n, sck, loopback, cs, irq, dma, dma_tx, dma_rx;
    sc_signal<sc_uint<32>> frequency;

    SC_CTOR(spi_register_test)
    {
        socket.bind(top.tgt_socket);
        top.rst_n(rst_n);
        top.clk_sys_freq(frequency);
        top.sck(sck);
        top.mosi(loopback);
        top.miso(loopback);
        top.cs_n(cs);
        top.irq(irq);
        top.dma_req(dma);
        top.dma_tx_req(dma_tx);
        top.dma_rx_req(dma_rx);
        SC_THREAD(run);
    }

    tlm_response_status transfer(tlm_command command, unsigned address, uint32_t& data)
    {
        tlm_generic_payload transaction;
        transaction.set_command(command);
        transaction.set_address(address);
        transaction.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        transaction.set_data_length(sizeof(data));
        sc_time delay = SC_ZERO_TIME;
        socket->b_transport(transaction, delay);
        return transaction.get_response_status();
    }

    uint32_t access(tlm_command command, unsigned address, uint32_t data = 0)
    {
        const auto response = transfer(command, address, data);
        sc_assert(response == TLM_OK_RESPONSE);
        return data;
    }

    void run()
    {
        frequency.write(8'000'000);
        wait(1, SC_NS);
        rst_n.write(true);
        wait(1, SC_NS);
        check_transport_contract();

        access(TLM_WRITE_COMMAND, SPI_IER, tc_bit);
        access(TLM_WRITE_COMMAND, SPI_DMAER, txe_bit);
        access(TLM_WRITE_COMMAND, SPI_BRR, 2);
        access(TLM_WRITE_COMMAND, SPI_WIDTH, 4);
        wait(1, SC_NS);
        sc_assert(access(TLM_READ_COMMAND, SPI_SR) == txe_bit);
        sc_assert(!irq.read() && dma.read());
        sc_assert(dma_tx.read() && !dma_rx.read());

        access(TLM_WRITE_COMMAND, SPI_TXDR, 0xA);
        wait(1, SC_NS);
        sc_assert(!dma.read());
        sc_assert(access(TLM_READ_COMMAND, SPI_SR) == 0);

        access(TLM_WRITE_COMMAND, SPI_DMAER, rxne_bit);
        access(TLM_WRITE_COMMAND, SPI_CR1, 0x9);
        wait(2, SC_US);
        sc_assert(irq.read() && dma.read());
        sc_assert(!dma_tx.read() && dma_rx.read());
        sc_assert(access(TLM_READ_COMMAND, SPI_SR) == (txe_bit | rxne_bit | tc_bit));
        wait(1, SC_NS);
        sc_assert(!irq.read() && dma.read());
        sc_assert(access(TLM_READ_COMMAND, SPI_SR) == (txe_bit | rxne_bit));

        sc_assert(access(TLM_READ_COMMAND, SPI_RXDR) == 0xA);
        wait(1, SC_NS);
        sc_assert(!dma.read());
        sc_assert(access(TLM_READ_COMMAND, SPI_SR) == txe_bit);
        wait(1, SC_NS);

        run_dma_burst(0x8);  // CPHA=0
        run_dma_burst(0xC);  // CPHA=1
        run_dma_burst(0xA);  // CPOL=1, CPHA=0
        run_dma_burst(0xE);  // CPOL=1, CPHA=1
        run_same_delta_accesses();
        run_rx_overflow();

        access(TLM_WRITE_COMMAND, SPI_CR1, 0xE);
        wait(1, SC_NS);
        access(TLM_WRITE_COMMAND, SPI_TXDR, 0xA);
        access(TLM_WRITE_COMMAND, SPI_TXDR, 0xB);
        rst_n.write(false);
        wait(1, SC_NS);
        sc_assert(!irq.read() && !dma.read() && !dma_tx.read() && !dma_rx.read());
        rst_n.write(true);
        wait(1, SC_NS);
        sc_assert(access(TLM_READ_COMMAND, SPI_SR) == txe_bit);
    }

    void check_transport_contract()
    {
        tlm_generic_payload transaction;
        uint32_t data = 0;
        transaction.set_address(SPI_CR1);
        transaction.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        transaction.set_data_length(sizeof(data));
        sc_time delay(5, SC_NS);

        transaction.set_command(TLM_IGNORE_COMMAND);
        socket->b_transport(transaction, delay);
        sc_assert(transaction.get_response_status() == TLM_COMMAND_ERROR_RESPONSE);

        transaction.set_command(TLM_READ_COMMAND);
        transaction.set_data_ptr(nullptr);
        socket->b_transport(transaction, delay);
        sc_assert(transaction.get_response_status() == TLM_GENERIC_ERROR_RESPONSE);

        transaction.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        socket->b_transport(transaction, delay);
        sc_assert(transaction.is_response_ok());
        sc_assert(delay == sc_time(5, SC_NS));
    }

    void run_dma_burst(unsigned control)
    {
        access(TLM_WRITE_COMMAND, SPI_CR1, control);
        access(TLM_WRITE_COMMAND, SPI_DMAER, txe_bit | rxne_bit);
        wait(1, SC_NS);
        sc_assert(dma_tx.read() && !dma_rx.read());

        // DMA 可在一个 delta 内提交整批写入。
        for (uint32_t word = 1; word <= FIFO_DEPTH; ++word)
            access(TLM_WRITE_COMMAND, SPI_TXDR, word);
        uint32_t excess = 0xF;
        const auto response = transfer(TLM_WRITE_COMMAND, SPI_TXDR, excess);
        sc_assert(response == TLM_GENERIC_ERROR_RESPONSE);
        wait(1, SC_NS);
        sc_assert((access(TLM_READ_COMMAND, SPI_SR) & bit(SPI_SR_TX_FULL_BIT)) != 0);
        sc_assert(!dma_tx.read() && !dma_rx.read() && !dma.read());
        const auto full_response = transfer(TLM_WRITE_COMMAND, SPI_TXDR, excess);
        sc_assert(full_response == TLM_GENERIC_ERROR_RESPONSE);

        access(TLM_WRITE_COMMAND, SPI_CR1, control | 1u);
        for (uint32_t expected = 1; expected <= FIFO_DEPTH; ++expected)
        {
            if (!dma_rx.read())
                wait(sc_time(2, SC_US), dma_rx.posedge_event());
            sc_assert(dma_rx.read() && dma.read());
            if (expected == 1)
                sc_assert(!dma_tx.read());
            if (expected == FIFO_DEPTH)
                sc_assert(dma_tx.read());
            const uint32_t received = access(TLM_READ_COMMAND, SPI_RXDR);
            sc_assert(received == expected);
            wait(1, SC_NS);
            sc_assert(!dma_rx.read());
        }
        sc_assert(dma_tx.read() && dma.read());
        wait(1, SC_US);
    }

    void run_same_delta_accesses()
    {
        access(TLM_WRITE_COMMAND, SPI_CR1, 0xC);
        wait(1, SC_NS);
        for (uint32_t word = 1; word <= FIFO_DEPTH - 2; ++word)
            access(TLM_WRITE_COMMAND, SPI_TXDR, word);
        wait(1, SC_NS);
        access(TLM_WRITE_COMMAND, SPI_TXDR, FIFO_DEPTH - 1);
        access(TLM_WRITE_COMMAND, SPI_TXDR, FIFO_DEPTH);
        uint32_t excess = 0xF;
        const auto response = transfer(TLM_WRITE_COMMAND, SPI_TXDR, excess);
        sc_assert(response == TLM_GENERIC_ERROR_RESPONSE);

        access(TLM_WRITE_COMMAND, SPI_CR1, 0xD);
        wait(9, SC_US);
        sc_assert(dma_rx.read());
        const unsigned pending = access(TLM_READ_COMMAND, SPI_SR);
        const unsigned cleared = access(TLM_READ_COMMAND, SPI_SR);
        sc_assert((pending & (tc_bit | bit(SPI_SR_RX_FULL_BIT))) ==
                  (tc_bit | bit(SPI_SR_RX_FULL_BIT)));
        sc_assert(cleared == (pending & ~tc_bit));

        for (uint32_t expected = 1; expected <= FIFO_DEPTH; ++expected)
        {
            const uint32_t received = access(TLM_READ_COMMAND, SPI_RXDR);
            sc_assert(received == expected);
        }
        const uint32_t empty = access(TLM_READ_COMMAND, SPI_RXDR);
        sc_assert(empty == 0);
        wait(1, SC_NS);
        sc_assert(!irq.read() && !dma_rx.read() && dma_tx.read());
    }

    void run_rx_overflow()
    {
        access(TLM_WRITE_COMMAND, SPI_IER, ovr_bit);
        for (uint32_t word = 1; word <= FIFO_DEPTH; ++word)
            access(TLM_WRITE_COMMAND, SPI_TXDR, word);
        wait(9, SC_US);
        sc_assert(dma_rx.read() && !irq.read());

        access(TLM_WRITE_COMMAND, SPI_TXDR, 0xF);
        wait(2, SC_US);
        sc_assert(irq.read());
        const unsigned pending = access(TLM_READ_COMMAND, SPI_SR);
        sc_assert((pending & ovr_bit) != 0);

        for (uint32_t expected = 1; expected <= FIFO_DEPTH; ++expected)
        {
            const uint32_t received = access(TLM_READ_COMMAND, SPI_RXDR);
            sc_assert(received == expected);
        }
        const uint32_t empty = access(TLM_READ_COMMAND, SPI_RXDR);
        sc_assert(empty == 0);
        wait(1, SC_NS);
        sc_assert(!irq.read() && !dma_rx.read() && dma_tx.read());
    }
};
} // namespace

int sc_main(int, char**)
{
    status_irq_dma_test module_test{"modules"};
    spi_register_test register_test{"registers"};
    sc_start();
    return 0;
}
