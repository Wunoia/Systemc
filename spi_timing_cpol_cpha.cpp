#include "spi_timing_cpol_cpha.h"

SC_HAS_PROCESS(spi_timing_cpol_cpha);
spi_timing_cpol_cpha::spi_timing_cpol_cpha(sc_module_name name)
    : sc_module(name)
{
    SC_THREAD(timing_proc);
    sensitive << rst_n;
}

void spi_timing_cpol_cpha::timing_proc()
{
    bool sck_cur;

    // [ERROR] 原实现 while(clk_en) 无限循环，且没有位计数，SCK 永不停；
    //         CPHA=0 时 MSB 输出时序也不对。
    // [FIX] 等待 start_trig，检查 clk_en && div_valid，
    //       按 trans_width 产生固定个数 SCK 周期后停止。
    while (true)
    {
        if (!rst_n.read())
        {
            sck.write(cpol.read());
            sample_en.write(false);
            shift_en.write(false);
            wait(rst_n.posedge_event());
            continue;
        }

        sck_cur = cpol.read();
        sck.write(sck_cur);
        sample_en.write(false);
        shift_en.write(false);

        wait(start_trig.posedge_event() | rst_n.negedge_event());
        if (!rst_n.read()) continue;

        if (!clk_en.read() || !div_valid.read()) continue;

        double freq_val = sck_freq.read().to_double();
        if (freq_val <= 0.0) continue;

        sc_time half_t = sc_time(1.0 / (2.0 * freq_val), SC_SEC);
        sc_uint<6> width = trans_width.read();
        if (width == 0) continue;

        if (cpha.read() == 0)
        {
            // CPHA=0：第一个 SCK 边沿之前先输出 MSB
            shift_en.write(true);
            wait(SC_ZERO_TIME);
            shift_en.write(false);

            for (sc_uint<6> i = 0; i < width; ++i)
            {
                wait(half_t);
                if (!rst_n.read()) break;

                sck_cur = !sck_cur;
                sck.write(sck_cur);
                sample_en.write(true);
                wait(SC_ZERO_TIME);
                sample_en.write(false);

                wait(half_t);
                if (!rst_n.read()) break;

                sck_cur = !sck_cur;
                sck.write(sck_cur);

                if (i + 1 < width)
                {
                    shift_en.write(true);
                    wait(SC_ZERO_TIME);
                    shift_en.write(false);
                }
            }
        }
        else
        {
            // CPHA=1：第一个边沿移位输出，第二个边沿采样
            for (sc_uint<6> i = 0; i < width; ++i)
            {
                wait(half_t);
                if (!rst_n.read()) break;

                sck_cur = !sck_cur;
                sck.write(sck_cur);
                shift_en.write(true);
                wait(SC_ZERO_TIME);
                shift_en.write(false);

                wait(half_t);
                if (!rst_n.read()) break;

                sck_cur = !sck_cur;
                sck.write(sck_cur);
                sample_en.write(true);
                wait(SC_ZERO_TIME);
                sample_en.write(false);
            }
        }

        // 传输结束，SCK 回到 CPOL
        sck_cur = cpol.read();
        sck.write(sck_cur);
    }
}