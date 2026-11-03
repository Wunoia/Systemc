#include "spi_clk_div_width.h"

#include <iostream>

SC_MODULE(ClkDivWidthTestbench)
{
    sc_signal<bool> rst_n;
    sc_signal<sc_uint<32>> clk_sys_freq;
    sc_signal<sc_uint<8>> div_cfg;
    sc_signal<sc_uint<6>> data_width_cfg;

    sc_signal<sc_uint<32>> sck_freq;
    sc_signal<sc_uint<6>> trans_width;
    sc_signal<bool> div_valid;

    spi_clk_div_width dut;

    int failures = 0;

    SC_CTOR(ClkDivWidthTestbench)
        : dut("dut")
    {
        dut.rst_n(rst_n);
        dut.clk_sys_freq(clk_sys_freq);
        dut.div_cfg(div_cfg);
        dut.data_width_cfg(data_width_cfg);
        dut.sck_freq(sck_freq);
        dut.trans_width(trans_width);
        dut.div_valid(div_valid);

        SC_THREAD(run);
    }

    void check(bool condition, const char* message)
    {
        std::cout << (condition ? "[PASS] " : "[FAIL] ")
                  << message
                  << " at " << sc_time_stamp()
                  << std::endl;

        if (!condition)
            ++failures;
    }

    void check_value(uint32_t actual, uint32_t expected,
                     const char* message)
    {
        const bool ok = actual == expected;

        std::cout << (ok ? "[PASS] " : "[FAIL] ")
                  << message
                  << ": actual=" << actual
                  << ", expected=" << expected
                  << std::endl;

        if (!ok)
            ++failures;
    }

    void apply(bool reset,
               uint32_t system_hz,
               uint32_t divider,
               uint32_t width)
    {
        rst_n.write(reset);
        clk_sys_freq.write(system_hz);
        div_cfg.write(divider);
        data_width_cfg.write(width);

        wait(1, SC_NS);
    }

    void expect_invalid(const char* message)
    {
        check(!div_valid.read(), message);
        check_value(sck_freq.read().to_uint(), 0,
                    "invalid config clears SCK frequency");
        check_value(trans_width.read().to_uint(), 0,
                    "invalid config clears transfer width");
    }

    void run()
    {
        std::cout << "=== SPI clock divider and width test ==="
                  << std::endl;

        apply(false, 50000000, 50, 8);
        expect_invalid("reset clears divider output");

        apply(true, 50000000, 50, 8);
        check(div_valid.read(),
              "50 MHz / 50 / 8 bit is valid");
        check_value(sck_freq.read().to_uint(), 1000000,
                    "50 MHz / 50 produces 1 MHz SCK");
        check_value(trans_width.read().to_uint(), 8,
                    "8 bit width is forwarded");

        apply(true, 50000000, 100, 16);
        check(div_valid.read(),
              "16 bit configuration is valid");
        check_value(sck_freq.read().to_uint(), 500000,
                    "50 MHz / 100 produces 500 kHz SCK");
        check_value(trans_width.read().to_uint(), 16,
                    "16 bit width is forwarded");

        apply(true, 64000000, 32, 32);
        check(div_valid.read(),
              "32 bit configuration is valid");
        check_value(sck_freq.read().to_uint(), 2000000,
                    "64 MHz / 32 produces 2 MHz SCK");
        check_value(trans_width.read().to_uint(), 32,
                    "32 bit width is forwarded");

        apply(true, 32000000, 16, 4);
        check(div_valid.read(),
              "4 bit configuration is valid");
        check_value(sck_freq.read().to_uint(), 2000000,
                    "32 MHz / 16 produces 2 MHz SCK");
        check_value(trans_width.read().to_uint(), 4,
                    "4 bit width is forwarded");

        apply(true, 50000000, 0, 8);
        expect_invalid("zero divider is rejected");

        apply(true, 50000000, 50, 7);
        expect_invalid("unsupported 7 bit width is rejected");

        apply(true, 50000000, 50, 33);
        expect_invalid("unsupported 33 bit width is rejected");

        apply(true, 100, 200, 8);
        expect_invalid("SCK below 1 Hz is rejected");

        if (failures == 0) {
            std::cout << "=== ALL CLOCK DIVIDER TESTS PASSED ==="
                      << std::endl;
        } else {
            std::cout << "=== CLOCK DIVIDER TESTS FAILED: "
                      << failures << " ==="
                      << std::endl;
        }

        sc_stop();
    }
};

int sc_main(int, char**)
{
    ClkDivWidthTestbench tb("tb");
    sc_start();
    return tb.failures == 0 ? 0 : 1;
}
