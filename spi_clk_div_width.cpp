#include "spi_clk_div_width.h"

namespace
{

    bool is_supported_width(uint32_t width)
    {
        return width == 4 || width == 8 ||
               width == 16 || width == 32;
    }

} // namespace

spi_clk_div_width::spi_clk_div_width(sc_core::sc_module_name name)
    : sc_module(name),
      rst_n("rst_n"),
      clk_sys_freq("clk_sys_freq"),
      div_cfg("div_cfg"),
      data_width_cfg("data_width_cfg"),
      sck_freq("sck_freq"),
      trans_width("trans_width"),
      div_valid("div_valid")
{
    SC_METHOD(calc_proc);
    sensitive << rst_n
              << clk_sys_freq
              << div_cfg
              << data_width_cfg;
}

void spi_clk_div_width::calc_proc()
{
    const uint32_t system_hz = clk_sys_freq.read().to_uint();
    const uint32_t divider = div_cfg.read().to_uint();
    const uint32_t width = data_width_cfg.read().to_uint();

    if (!rst_n.read() || system_hz == 0 || divider == 0 || !is_supported_width(width))
    {
        sck_freq.write(0);
        trans_width.write(0);
        div_valid.write(false);
        return;
    }
    const uint32_t calculated_sck_hz = system_hz / divider;
    if (calculated_sck_hz == 0)
    {
        sck_freq.write(0);
        trans_width.write(0);
        div_valid.write(false);
        return;
    }
    sck_freq.write(calculated_sck_hz);
    trans_width.write(width);
    div_valid.write(true);
}