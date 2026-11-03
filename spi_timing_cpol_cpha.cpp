#include "spi_timing_cpol_cpha.h"

SC_HAS_PROCESS(spi_timing_cpol_cpha);
spi_timing_cpol_cpha::spi_timing_cpol_cpha(sc_module_name name)
    : sc_module(name)
{
    SC_THREAD(timing_proc);
    sensitive << rst_n;
}