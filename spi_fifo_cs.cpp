// 数据缓存（TX/RX FIFO）与片选信号控制
// ---------------------------------------------------------------
// 时序契约（与顶层 spi_controller_top、移位模块、时序模块配合）：
//  1. tx_wen / rx_ren / start_bit 由顶层在 TLM 访问中抬高一拍，
//     顶层 clear_pulse_proc 在下一个 delta 清除；本模块在事件所在
//     delta 内读取电平，绝不额外 wait，避免漏采或重复计数。
//  2. 帧启动流程：弹出 TX 队首 -> 写 tx_parallel -> 下一 delta 拉高
//     start_trig 一个 delta，保证移位模块在 start_trig 上升沿采到的
//     tx_parallel 已经是本帧数据（无 delta 竞争）。
//  3. rx_rdata 始终镜像 RX 队首，b_transport 里“先读 RXDR 再写 rx_ren”
//     的读-弹语义因此成立。
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

        if (rx_ren.event() && rx_ren.read())
            handle_rx_read();

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

    update_flags();
    update_rx_head();
    tx_parallel.write(0);
    ovr_err.write(false);
    apply_cs();
}

//================================================================
// 事件处理
//================================================================
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

    update_flags();

    // 序列已开启但上一帧已收完（处于等待数据态）时，写入立刻起一帧
    if (m_state == ST_ACTIVE && start_bit.read())
        start_frame();
}

void spi_fifo_cs::handle_rx_read()
{
    sc_uint<32> data = 0;

    if (!pop_rx(data))
        return;                     // 空读：rx_rdata 保持 0

    update_flags();
    update_rx_head();

    // RX 被读空后，溢出标志自动解除，便于软件恢复
    if (m_rx_cnt == 0 && m_ovr)
    {
        m_ovr = false;
        ovr_err.write(false);
    }
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
        ovr_err.write(true);        // RX 满：丢弃本帧，标志粘滞到 RX 被读空
        SC_REPORT_WARNING("spi_fifo_cs", "RX FIFO overflow, frame dropped");
    }

    update_flags();
    update_rx_head();

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
