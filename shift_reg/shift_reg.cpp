#include "../headfile/spi_shift_reg.h"

spi_shift_reg::spi_shift_reg(sc_module_name name): sc_module(name){
    SC_THREAD(shift_proc);
}
void spi_shift_reg::shift_proc(){//发送寄存器、接收寄存器、本帧位宽、接收位数计数器。
sc_uint<32> tx_reg = 0;
sc_uint<32> rx_reg = 0;
unsigned int frame_width = 0;
unsigned int rx_count = 0;

mosi.write(false);
rx_parallel.write(0);
trans_done.write(false);
busy.write(false);

if(!rst_n.read() == 1){
    tx_reg = 0;
    rx_reg = 0;
    frame_width = 0;
    rx_count = 0;
}
while(1){
    rx_reg = 0;
    rx_count = 0;
    
    wait(start_trig.posedge_event() | rst_n.negedge_event());
    if(rst_n.read() == 0){
        tx_reg = 0;
        rx_reg = 0;
        frame_width = 0;
        rx_count = 0;
    }else{
        tx_reg = tx_parallel.read();
        frame_width = trans_width.read();
        trans_done.write(0);
        busy.write(1);

        while(rx_count < frame_width){
            wait(sample_en.posedge_event() | shift_en.posedge_event() 
                                           | rst_n.negedge_event());
            if(rst_n == 0){break;}else{
            if(sample_en.posedge()){
            rx_reg = rx_reg << 1;
            rx_reg += miso_bit.read();
            rx_count++;
            }
            if(shift_en.posedge()){
                mosi = tx_reg[frame_width - 1];
                tx_reg = tx_reg << 1;
            }
            }
        }
        if(rx_count == frame_width){
            rx_parallel.write(rx_reg);
        }
            busy.write(0);
        }
    }
}