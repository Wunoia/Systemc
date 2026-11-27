cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/c++.exe" CACHE FILEPATH "" FORCE)

project(spi_systemc_sim LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "C:/systemc" CACHE PATH "" FORCE)

find_package(SystemCLanguage CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(main
    main.cpp
    spi_clk_div_width.cpp
    spi_timing_cpol_cpha.cpp
    spi_status_gen.cpp
    spi_shift_reg.cpp
    spi_fifo_cs.cpp
    spi_irq_dma.cpp
    spi_controller_top.cpp
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/headfile
)

target_link_libraries(main PRIVATE SystemC::systemc)

target_link_options(main PRIVATE -static)

set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
