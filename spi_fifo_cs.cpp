// 数据缓存（TX/RX FIFO）与片选信号控制
// ---------------------------------------------------------------
// 时序契约（与顶层 spi_controller_top、移位模块、时序模块配合）：
//  1. TLM 访问直接写入 TX FIFO 或读取 RX FIFO。单独使用本模块时，
//     tx_wen 和 rx_ren 的上升沿也可触发一次读写。
//  2. 帧启动流程：弹出 TX 队首 -> 写 tx_parallel -> 下一 delta 拉高
//     start_trig 一个 delta，保证移位模块在 start_trig 上升沿采到的
//     tx_parallel 已经是本帧数据（无 delta 竞争）。
//  3. FIFO 输出信号由主线程更新，rx_rdata 镜像 RX 队首。
//  4. cs_n 是电平信号，随状态机与 cs_cfg 立即更新。
// ---------------------------------------------------------------

#include "spi_fifo_cs.h"

#ifdef FIFO_CS_DEBUG
#include <iostream>
#define FCSDBG(msg) \
    (std::cout << "[fifo_cs " << sc_core::sc_time_stamp() << "] " << msg << std::endl)
#else
#define FCSDBG(msg) do { } while (0)
#endif

namespace
{
    unsigned int next_index(unsigned int idx)
    {
        return (idx + 1u) % FIFO_DEPTH;
    }
}

spi_fifo_cs::spi_fifo_cs(sc_core::sc_module_name name)
    : sc_module(name),
      rst_n("rst_n"),
      tx_wdata("tx_wdata"),
      tx_wen("tx_wen"),
      rx_ren("rx_ren"),
      rx_parallel("rx_parallel"),
      trans_done("trans_done"),
      cs_cfg("cs_cfg"),
      start_bit("start_bit"),
      tx_parallel("tx_parallel"),
      start_trig("start_trig"),
      rx_rdata("rx_rdata"),
      tx_fifo_empty("tx_fifo_empty"),
      tx_fifo_full("tx_fifo_full"),
      rx_fifo_empty("rx_fifo_empty"),
      rx_fifo_full("rx_fifo_full"),
      ovr_err("ovr_err"),
      cs_n("cs_n"),
      m_state(ST_IDLE),
      m_cs_on(false),
      m_ovr(false),
      m_start_evt("m_start_evt"),
      m_tx_wp(0),
      m_tx_rp(0),
      m_tx_cnt(0),
      m_rx_wp(0),
      m_rx_rp(0),
      m_rx_cnt(0)
{
    SC_THREAD(fifo_proc);
    sensitive << rst_n;

    SC_THREAD(pulse_proc);      // 无静态敏感表：只由 m_start_evt 唤醒
}

//================================================================
// 主线程：FIFO 读写 + 片选状态机
//================================================================
void spi_fifo_cs::fifo_proc()
{
    reset_state();

    while (true)
    {
        if (!rst_n.read())
        {
            reset_state();
            wait(rst_n.posedge_event());
            continue;
        }

        // 每轮只处理本次唤醒对应的那些事件；电平门控防止重复处理
        if (tx_wen.event() && tx_wen.read())
            handle_tx_write();
        if (m_reg_tx_written.triggered())
            handle_tx_available();

        if (rx_ren.event() && rx_ren.read())
            handle_rx_read();
        if (m_reg_rx_read.triggered())
            update_rx_outputs();

        if (start_bit.event())
            handle_start_bit();

        if (trans_done.event() && trans_done.read())
            handle_frame_done();

        if (cs_cfg.event())
            apply_cs();

        wait(tx_wen.value_changed_event()
             | rx_ren.value_changed_event()
             | start_bit.value_changed_event()
             | trans_done.value_changed_event()
             | cs_cfg.value_changed_event()
             | m_reg_tx_written
             | m_reg_rx_read
             | rst_n.negedge_event());
    }
}

//================================================================
// 脉冲整形：start_trig 只由本线程驱动，宽度一个 delta
//================================================================
void spi_fifo_cs::pulse_proc()
{
    start_trig.write(false);

    while (true)
    {
        wait(m_start_evt);

        if (!rst_n.read())
        {
            start_trig.write(false);
            continue;
        }

        start_trig.write(true);
        wait(SC_ZERO_TIME);
        start_trig.write(false);
    }
}

//================================================================
// 复位 / 初始化
//================================================================
void spi_fifo_cs::reset_state()
{
    m_tx_wp = 0;
    m_tx_rp = 0;
    m_tx_cnt = 0;
    m_rx_wp = 0;
    m_rx_rp = 0;
    m_rx_cnt = 0;

    for (unsigned int i = 0; i < FIFO_DEPTH; ++i)
    {
        m_tx_mem[i] = 0;
        m_rx_mem[i] = 0;
    }

    m_state = ST_IDLE;
    m_cs_on = false;
    m_ovr   = false;

    update_rx_outputs();
    tx_parallel.write(0);
    apply_cs();
}

//================================================================
// 事件处理
//================================================================
bool spi_fifo_cs::write_tx_data(const sc_uint<32>& data)
{
    if (!rst_n.read() || !push_tx(data))
        return false;

    m_reg_tx_written.notify(SC_ZERO_TIME);
    return true;
}

sc_uint<32> spi_fifo_cs::read_rx_data()
{
    sc_uint<32> data = 0;
    if (rst_n.read() && pop_rx(data))
        m_reg_rx_read.notify(SC_ZERO_TIME);
    return data;
}

void spi_fifo_cs::handle_tx_write()
{
    const sc_uint<32> data = tx_wdata.read();
    FCSDBG("tx_write data=0x" << std::hex << data.to_uint() << std::dec
           << " cnt=" << m_tx_cnt << " state=" << int(m_state));

    if (!push_tx(data))
    {
        // TX FIFO 已满：硬件丢弃该次写入，软件应先查 SR 的 TXFULL 位
        SC_REPORT_WARNING("spi_fifo_cs", "TX FIFO is full, write dropped");
        return;
    }

    handle_tx_available();
}

void spi_fifo_cs::handle_tx_available()
{
    update_flags();
    // 序列已开启但上一帧已收完（处于等待数据态）时，写入立刻起一帧
    if (m_state == ST_ACTIVE && start_bit.read())
        start_frame();
}

void spi_fifo_cs::handle_rx_read()
{
    sc_uint<32> data = 0;

    if (pop_rx(data))
        update_rx_outputs();
}

void spi_fifo_cs::handle_start_bit()
{
    FCSDBG("start_bit=" << start_bit.read() << " state=" << int(m_state)
           << " tx_cnt=" << m_tx_cnt);
    if (start_bit.read())
    {
        if (m_state != ST_IDLE)
            return;                 // 序列进行中再次置位：忽略，软件应先查 BUSY

        m_state = ST_ACTIVE;
        m_cs_on = true;
        apply_cs();                 // 片选领先 SCK 有效

        if (m_tx_cnt > 0)
            start_frame();
    }
    else
    {
        if (m_state == ST_ACTIVE)
        {
            m_state = ST_IDLE;      // 放弃等待：释放片选，数据留在 FIFO 里
            m_cs_on = false;
            apply_cs();
        }
        // ST_FRAME：当前帧收发完再在 handle_frame_done 里释放片选
    }
}

void spi_fifo_cs::handle_frame_done()
{
    FCSDBG("trans_done state=" << int(m_state) << " tx_cnt=" << m_tx_cnt
           << " rx_cnt=" << m_rx_cnt);
    if (m_state != ST_FRAME)
        return;

    if (!push_rx(rx_parallel.read()))
    {
        m_ovr = true;
        SC_REPORT_WARNING("spi_fifo_cs", "RX FIFO overflow, frame dropped");
    }

    update_rx_outputs();

    if (start_bit.read())
    {
        if (m_tx_cnt > 0)
        {
            start_frame();          // 连续传输：片选保持有效
        }
        else
        {
            m_state = ST_ACTIVE;    // 片选保持有效，等软件补数据
            m_cs_on = true;
            apply_cs();
        }
    }
    else
    {
        m_state = ST_IDLE;          // 帧结束，释放片选
        m_cs_on = false;
        apply_cs();
    }
}

void spi_fifo_cs::start_frame()
{
    sc_uint<32> data = 0;

    if (!pop_tx(data))
        return;                     // TX 空：不起帧

    FCSDBG("start_frame data=0x" << std::hex << data.to_uint() << std::dec
           << " left=" << m_tx_cnt);
    tx_parallel.write(data);        // 先把并行数据摆好
    update_flags();

    m_state = ST_FRAME;
    m_cs_on = true;
    apply_cs();

    m_start_evt.notify(SC_ZERO_TIME);   // 下一 delta 产生 start_trig 上升沿
}

//================================================================
// 片选驱动
//================================================================
void spi_fifo_cs::apply_cs()
{
    const unsigned int cfg = cs_cfg.read().to_uint();
    const bool force_off  = ((cfg >> CSCR_CS_OFF_POS) & 0x1u) != 0;
    const bool active_high = ((cfg >> CSCR_CS_POL_POS) & 0x1u) != 0;
    const bool active = m_cs_on && !force_off;

    cs_n.write(active_high ? active : !active);
}

//================================================================
// FIFO 基本操作
//================================================================
bool spi_fifo_cs::push_tx(const sc_uint<32>& data)
{
    if (m_tx_cnt >= FIFO_DEPTH)
        return false;

    m_tx_mem[m_tx_wp] = data;
    m_tx_wp = next_index(m_tx_wp);
    ++m_tx_cnt;
    return true;
}

bool spi_fifo_cs::pop_tx(sc_uint<32>& data)
{
    if (m_tx_cnt == 0)
        return false;

    data = m_tx_mem[m_tx_rp];
    m_tx_rp = next_index(m_tx_rp);
    --m_tx_cnt;
    return true;
}

bool spi_fifo_cs::push_rx(const sc_uint<32>& data)
{
    if (m_rx_cnt >= FIFO_DEPTH)
        return false;

    m_rx_mem[m_rx_wp] = data;
    m_rx_wp = next_index(m_rx_wp);
    ++m_rx_cnt;
    return true;
}

bool spi_fifo_cs::pop_rx(sc_uint<32>& data)
{
    if (m_rx_cnt == 0)
        return false;

    data = m_rx_mem[m_rx_rp];
    m_rx_rp = next_index(m_rx_rp);
    --m_rx_cnt;
    if (m_rx_cnt == 0)
        m_ovr = false;
    return true;
}

//================================================================
// 输出刷新
//================================================================
void spi_fifo_cs::update_flags()
{
    tx_fifo_empty.write(m_tx_cnt == 0);
    tx_fifo_full.write(m_tx_cnt >= FIFO_DEPTH);
    rx_fifo_empty.write(m_rx_cnt == 0);
    rx_fifo_full.write(m_rx_cnt >= FIFO_DEPTH);
}

void spi_fifo_cs::update_rx_head()
{
    if (m_rx_cnt == 0)
        rx_rdata.write(0);
    else
        rx_rdata.write(m_rx_mem[m_rx_rp]);
}

void spi_fifo_cs::update_rx_outputs()
{
    update_flags();
    update_rx_head();
    ovr_err.write(m_ovr);
}
