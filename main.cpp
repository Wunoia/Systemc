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

        top.clk_sys_freq(clk_sys_freq);  // 漏绑端口，导致报E109 port not bound

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

    if(trans.get_response_status() != TLM_OK_RESPONSE) cout << "Error: write32 failed" << endl;
    // get_response_status误写成了get_command
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

    if(trans.get_response_status() != TLM_OK_RESPONSE) cout << "Error: read32 failed" << endl;
    // get_response_status误写成了get_command

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

    // write32(SPI_CSCR,  0X09); 启动位写到了错误寄存器
    write32(SPI_CR1,  0X09);  // fixed
    wait(2,SC_NS);

    if(cs_n.read() == false) cout << "CS = 0" << endl;
    if(wait_tc(20)) cout << "传输完成" << endl;
    if((read32(SPI_SR) & (1u << SPI_SR_RXNE_BIT)) != 0) cout << "收到数据后 RXNE=1" << endl;
    if(read32(SPI_RXDR) == 0x0000005Au) cout << "回环接收数据 = 0x5A" << endl;
    if((read32(SPI_SR) & (1u << SPI_SR_RXNE_BIT)) == 0) cout << "RX 读空后 RXNE=0" << endl;
    write32(SPI_CR1,0x08);
    wait(10,SC_NS);
    if(cs_n.read() == true) cout << "start 清零后 CS 自动释放(高)" << endl;
    cnt++;

    cout << "number of tests:"<< cnt << endl;
    cout << "cpol = 1,cpha = 1时传输测试" << endl;
    write32(SPI_WIDTH, 16);
    write32(SPI_CR1, 0x0Eu);             // CPOL=1, CPHA=1, clk_en=1
    write32(SPI_TXDR, 0x0000C3A5u);
    write32(SPI_CR1, 0x0Fu);             // start=1
    wait(3, SC_US);
    if(cs_n.read() == false) cout << "CS = 0" << endl;
    if(wait_tc(20)) cout << "传输完成" << endl;
    if(sck.read() == true) cout << "CPOL=1,CPHA=1时 SCK = 1" << endl;
    if(read32(SPI_RXDR) == 0x0000C3A5u) cout << "回环接收数据 = 0xC3A5" << endl;
    write32(SPI_CR1,0x0Eu);
    wait(10,SC_NS);
    if(cs_n.read() == true) cout << "start 清零后 CS 自动释放(高)" << endl;
    cnt++;

    cout << "number of tests:"<< cnt << endl;
    cout << "TX FIFO 测试" << endl;
    write32(SPI_CR1, 0x08);
    write32(SPI_WIDTH, 8);
    const uint32_t tx_frames[4] = {0x11u, 0x22u, 0x33u, 0x44u};
    for (int i = 0; i < 4; ++i)
        write32(SPI_TXDR, tx_frames[i]);
    if((read32(SPI_SR) & (1u << SPI_SR_TX_FULL_BIT)) == 0) cout << "4 帧写入后 TX FIFO 未满" << endl;
        write32(SPI_CR1, 0x09);
    int tc_cnt = 0;
    for (int i = 0; i < 600 && tc_cnt < 4; ++i)
    {
        if (read32(SPI_SR) & (1u << SPI_SR_TC_BIT))
            ++tc_cnt;
        wait(100, SC_NS);
    }
    if(tc_cnt == 4) cout << "4 帧传输完成" << endl;
    for(int i = 0; i < 4; ++i){
        if(read32(SPI_RXDR) == tx_frames[i]){
            cout << "回环接收数据 = 0x" << hex << tx_frames[i] << endl;
        }
    }
    write32(SPI_CR1,0x08);
    cnt++;

    cout << "number of tests:"<< cnt << endl;
    cout << "RX FIFO 溢出测试 " << endl;
    for (int i = 0; i < 8; ++i)
        write32(SPI_TXDR, static_cast<uint32_t>(i));
    if((read32(SPI_SR) & (1u << SPI_SR_TX_FULL_BIT)) != 0) cout << "写满 8 帧后 TXFULL=1" << endl;
    write32(SPI_CR1, 0x09);
    wait(90, SC_US);                     // 8 帧 x 8us = 64us，留足余量
    if((read32(SPI_SR) & (1u << SPI_SR_RX_FULL_BIT)) != 0) cout << "8 帧收完后 RXFULL=1"<< endl;
    write32(SPI_TXDR, 0x00000088u);      // 第 9 帧 -> RX 溢出
    wait(20, SC_US);
    if((read32(SPI_SR) & (1u << SPI_SR_OVR_BIT)) != 0) cout <<  "第 9 帧触发 OVR=1" << endl;
    for (int i = 0; i < 8; ++i)
        if(read32(SPI_RXDR) == static_cast<uint32_t>(i)) cout << "溢出后回环接收数据 = 0x" << hex << static_cast<uint32_t>(i) << endl;
    if((read32(SPI_SR) & (1u << SPI_SR_OVR_BIT)) == 0) cout << "RX 读空后 OVR 自动清除" << endl;
    write32(SPI_CR1, 0x08);
    cnt++;

    cout << "number of tests:"<< cnt << endl;
    cout << "中断 IER(TC) 与 DMA 请求 DMAER(TXE)" << endl;
    write32(SPI_IER, 0x0000u);
    write32(SPI_DMAER, 0x0001u);         // 使能 TXE 的 DMA 请求
    wait(10, SC_NS);
    if(dma_req.read() == true) cout << "TX FIFO 为空时产生 DMA 请求" << endl;
    write32(SPI_DMAER, 0x0000u);
    wait(10, SC_NS);
    if(dma_req.read() == false) cout << "关闭 DMAER 后请求撤销" << endl;

     write32(SPI_IER, 0x0004u);           // 使能 TC 中断
    write32(SPI_TXDR, 0x00000077u);
    write32(SPI_CR1, 0x09u);
    // 先直接观察 irq 电平；不能先读 SR，因为读 SR 会立即清除 TC 事件
    bool irq_seen = false;
    for (int i = 0; i < 300 && !irq_seen; ++i)
    {
        if (irq.read()) irq_seen = true;
        else wait(100, SC_NS);
    }
    if(irq_seen) cout << "TC 事件置位 irq" << endl;
    if((read32(SPI_SR) & (1u << SPI_SR_TC_BIT)) != 0) cout << "TC 状态位置位" << endl;
    wait(10, SC_NS);
    if(irq.read() == false ) cout << "清除事件后 irq 撤销" << endl;
    if(read32(SPI_RXDR) == 0x00000077u) cout << "TC 用例接收数据正确" << endl;
    write32(SPI_CR1, 0x08u);
    write32(SPI_IER, 0x0000u);
    cnt++;

    cout << "number of tests:"<< cnt << endl;
    cout << "片选控制 CSCR (CS_OFF / CS_POL)" << endl;
    write32(SPI_CSCR, 0x01u);            // CS_OFF = 1
    write32(SPI_TXDR, 0x00000033u);
    write32(SPI_CR1, 0x09u);
    wait(2, SC_US);
    if(cs_n.read() == true) cout << "CS_OFF=1 时传输中片选保持无效" << endl;
    if(wait_tc(20)) cout << "CS_OFF 用例传输完成" << endl;
    write32(SPI_CR1, 0x08u);

    write32(SPI_CSCR, 0x02u);            // CS_POL = 1 (高有效)
    write32(SPI_TXDR, 0x00000044u);
    write32(SPI_CR1, 0x09u);
    wait(2, SC_US);
    if(cs_n.read() == true) cout << "CS_POL=1 时传输中片选为高" << endl;
    if(wait_tc(20)) cout <<  "CS_POL 用例传输完成" << endl;
    write32(SPI_CR1, 0x08u);
    wait(10, SC_NS);
    if(cs_n.read() == false) cout << "CS_POL=1 时空闲片选为低" << endl;
    write32(SPI_CSCR, 0x00u);
    (void)read32(SPI_RXDR);
    (void)read32(SPI_RXDR);
    cnt++;

    cout << "number of tests:"<< cnt << endl;
    cout << "4bit 位宽帧传输" << endl;
    write32(SPI_WIDTH, 4);
    write32(SPI_CR1, 0x08u);
    write32(SPI_TXDR, 0x0000000Bu);
    write32(SPI_CR1, 0x09u);
    if(wait_tc(20)) cout << "4bit 用例传输完成" << endl;
    if(read32(SPI_RXDR) == 0x0000000Bu ) cout << "4bit 回环接收 = 0xB" << endl;
    write32(SPI_CR1, 0x08u);

}

bool tester::wait_tc(unsigned int timeout){
    for(unsigned int i = 0; i < timeout * 10u; i++){
        const uint32_t data = read32(SPI_SR);
        if(data & (1u << SPI_SR_TC_BIT)) return true;
        wait(100,SC_NS);
    }
    return false;
}

int sc_main(int argc, char* argv[]){
    tester tester("tests");
    sc_start();
    return 0;
}