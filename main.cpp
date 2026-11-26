#include "spi_controller_top.h"
#include <tlm_utils/simple_initiator_socket.h>

#include <iostream>
#include <string>

using namespace std;
using namespace sc_core;
using namespace sc_dt;
using namespace tlm;

static const uint32_t sys_clk_hz = 100000000u;

SC_MODULE(tester){
    tlm_utils::simple_initiator_socket<tester> init_socket;

    // 系统信号
    sc_signal<bool> rst_n;
    sc_signal<bool> sck;
    sc_signal<bool> spi_data;
    sc_signal<bool> cs_n;
    sc_signal<bool> irq;
    sc_signal<bool> dma_req;
    sc_signal<sc_uint<32>> clk_sys_freq;

    spi_controller_top top;

    SC_HAS_PROCESS(tester);

    SC_CTOR(tester):top("top"),init_socket("init_socket")
    {
        init_socket(top.tgt_socket);

        top.rst_n(rst_n);
        top.sck(sck);
        top.mosi(spi_data);
        top.miso(spi_data);
        top.cs_n(cs_n);
        top.irq(irq);
        top.dma_req(dma_req);

        SC_THREAD(run);
    }
private:
    void run();
    void write32(uint32_t addr, uint32_t data);
    void write16(uint32_t addr, uint16_t data);
    void write8(uint32_t addr, uint8_t data);
    void write4(uint32_t addr, uint8_t data);
    uint32_t read32(uint32_t addr);
    bool wait_tc(unsigned int timeout);
};

void tester::write32(uint32_t addr, uint32_t data){

    tlm_generic_payload trans;

    sc_time delay = SC_ZERO_TIME;

    unsigned char buf[4];
    buf[0] = static_cast<unsigned char>(data & 0xff);
    buf[1] = static_cast<unsigned char>((data >> 8) & 0xff);
    buf[2] = static_cast<unsigned char>((data >> 16) & 0xff);
    buf[3] = static_cast<unsigned char>((data >> 24) & 0xff);

    trans.set_command(TLM_WRITE_COMMAND);
    trans.set_address(static_cast<uint64>(addr));
    trans.set_data_ptr(buf);
    trans.set_data_length(4);
    trans.set_streaming_width(4);
    trans.set_byte_enable_ptr(nullptr);
    trans.set_byte_enable_length(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

    init_socket->b_transport(trans, delay);
    wait(delay);
    wait(1,SC_NS);

    if(trans.get_command() != TLM_OK_RESPONSE) cout << "Error: write32 failed" << endl;
}

uint32_t tester::read32(uint32_t addr){

    unsigned char buf[4] = {0,0,0,0};

    tlm_generic_payload trans;
    trans.set_command(TLM_READ_COMMAND);
    trans.set_address(static_cast<uint64>(addr));
    trans.set_data_ptr(buf);
    trans.set_data_length(4);
    trans.set_streaming_width(4);
    trans.set_byte_enable_ptr(nullptr);
    trans.set_byte_enable_length(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

    sc_time delay = SC_ZERO_TIME;
    init_socket->b_transport(trans, delay);
    wait(delay);
    wait(1, SC_NS);

    if(trans.get_command() != TLM_OK_RESPONSE) cout << "Error: read32 failed" << endl;

    return (static_cast<uint64>(buf[0]) 
        | static_cast<uint64>((buf[1] << 8)) 
        | static_cast<uint64>((buf[2] << 16))
        | static_cast<uint64>((buf[3] << 24)));
}

void tester::run(){
    int cnt = 1;
    cout << "start tester" << endl;
    cout << "number of tests:"<< cnt << endl;
    cout << "复位与读写测试" << endl;
    clk_sys_freq.write(sys_clk_hz);
    rst_n.write(false);
    wait(100,SC_NS);
    if(read32(SPI_SR) == 0x0000u) cout << " 复位期间 SR = 0x0000" << endl;
    if(!irq.read() && !dma_req.read()) cout << " 复位期间 irq/dam_req = 0" << endl;

    rst_n.write(true);
    wait(100,SC_NS);
    if((read32(SPI_SR) & 0x0001u) != 0) cout << " 复位后 SR = 0x0001;TX FIFO 为空" << endl;

    write32(SPI_BRR,   100u);
    write32(SPI_WIDTH, 8);
    write32(SPI_IER,   0x0005u);
    write32(SPI_DMAER, 0x0003u);
    write32(SPI_CSCR,  0x0002u);
    write32(SPI_CR1,   0x0008u);
    if(read32(SPI_BRR) == 100u) cout << " BRR 读回一致" << endl;
    if(read32(SPI_WIDTH) == 8u) cout << " WIDTH 读回一致" << endl;
    if(read32(SPI_IER) == 0x0005u) cout << " IER 读回一致" << endl;
    if(read32(SPI_DMAER) == 0x0003u) cout << " DMAER 读回一致" << endl;
    if(read32(SPI_CSCR) == 0x0002u) cout << " CSCR 读回一致" << endl;
    if(read32(SPI_CR1) == 0x0008u) cout << " CR1 读回一致" << endl;
    cnt++;

    {
        cout << "number of tests:"<< cnt << endl;
        cout << "读写长度非4字节的数据测试" << endl;
        unsigned char buf[2] = {0,0};
        tlm_generic_payload trans;
        trans.set_command(TLM_WRITE_COMMAND);
        trans.set_address(SPI_BRR);
        trans.set_data_ptr(buf);
        trans.set_data_length(2);
        trans.set_streaming_width(2);
        trans.set_byte_enable_ptr(nullptr);
        trans.set_byte_enable_length(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
        sc_time delay = SC_ZERO_TIME;
        init_socket->b_transport(trans, delay);
        if(trans.get_response_status() == TLM_GENERIC_ERROR_RESPONSE) {
            cout << " 测试: 读写操作不允许长度为非4字节的数据" << endl;
        }
        cnt++;
    }
    {
        cout << "number of tests:"<< cnt << endl;
        cout << "未知寄存器访问测试" << endl;
        unsigned char w;
        tlm_generic_payload trans;
        trans.set_command(TLM_WRITE_COMMAND);
        trans.set_address(0x1122);
        trans.set_data_ptr(&w);
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        trans.set_byte_enable_ptr(nullptr);
        trans.set_byte_enable_length(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
        sc_time delay = SC_ZERO_TIME;
        init_socket->b_transport(trans, delay);
        if(trans.get_response_status() == TLM_ADDRESS_ERROR_RESPONSE){
            cout << " 测试: 读写操作不允许访问未知的寄存器" << endl;
        }
        cnt++;
    }
    // 恢复默认
    write32(SPI_IER,0);
    write32(SPI_DMAER,0);
    write32(SPI_CSCR,0);
    write32(SPI_CR1,0X08);

    cout << "number of tests:"<< cnt << endl;
    cout << "单帧回环,FIFO,CS状态测试" << endl;
    write32(SPI_BRR,   100u);
    write32(SPI_WIDTH, 8);
    write32(SPI_CSCR,  0X00);
    write32(SPI_CR1,   0X08);  // clk_en = 1;cpol = 0;cpha = 0;start = 0;
    write32(SPI_TXDR, 0x0000005Au);

    if((read32(SPI_SR) & 0x0001u) == 0) cout << "写入TXDR后TXE = 0;" << endl;

    write32(SPI_CSCR,  0X09);
    wait(2,SC_NS);

    if(cs_n.read() == false) cout << "CS = 0" << endl;
    if(wait_tc(20)) cout << "传输完成" << endl;
    if((read32(SPI_SR) & (1u << SPI_SR_RXNE_BIT)) != 0) cout << "收到数据后 RXNE=1" << endl;
    if(read32(SPI_RXDR) == 0x0000005Au) cout << "回环接收数据 = 0x5A" << endl;
    if((read32(SPI_SR) & (1u << SPI_SR_RXNE_BIT)) == 0) cout << "RX 读空后 RXNE=0" << endl;
    write32(SPI_CR1,0x08);
    wait(10,SC_NS);
    if(cs_n.read() == true) cout << "start 清零后 CS 自动释放(高)" << endl;
}

bool tester::wait_tc(unsigned int timeout){
    for(unsigned int i = 0; i < timeout * 10u; i++){
        const uint32_t data = read32(SPI_SR);
        if(data & (1u << SPI_SR_TC_BIT)) return true;
        wait(100,SC_NS);
    }
    return false;
}