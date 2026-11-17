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

    while (true)
    {
        // 复位态
        if (!rst_n.read())           // 低有效复位
        {
            sck.write(cpol.read());  // 强制停留在idle状态
            sample_en.write(false);
            shift_en.write(false);
            wait(rst_n.posedge_event());  // 等待复位结束
            continue;                     // 后续会刷新sck_cur
        }

        // 空闲态
        sck_cur = cpol.read();         
        sck.write(sck_cur);
        sample_en.write(false);
        shift_en.write(false);

        // 启动等待
        wait(start_trig.posedge_event() | rst_n.negedge_event());
        if (!rst_n.read()) continue;

        // 门控，参数转换
        if (!clk_en.read() || !div_valid.read()) continue;

        double freq_val = sck_freq.read().to_double();
        if (freq_val <= 0.0) continue;

        sc_time half_t = sc_time(1.0 / (2.0 * freq_val), SC_SEC);
        sc_uint<6> width = trans_width.read();
        if (width == 0) continue;

        if (cpha.read() == 0)
        {
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
        sck_cur = cpol.read();
        sck.write(sck_cur);
    }
}