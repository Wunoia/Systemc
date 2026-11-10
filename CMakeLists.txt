cmake_minimum_required(VERSION 3.16)
project(Systemc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_PREFIX_PATH "$ENV{SYSTEMC_HOME}")
find_package(SystemCLanguage CONFIG REQUIRED)

add_library(spi_clk_div_width STATIC
    src/spi_clk_div_width.cpp
)

target_include_directories(spi_clk_div_width
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/headfile
)

target_link_libraries(spi_clk_div_width
    PUBLIC
        SystemC::systemc
)

add_executable(test_spi_clk_div_width
    tests/test_spi_clk_div_width.cpp
)

target_link_libraries(test_spi_clk_div_width
    PRIVATE
        spi_clk_div_width
)

enable_testing()
add_test(NAME spi_clk_div_width
         COMMAND test_spi_clk_div_width)
