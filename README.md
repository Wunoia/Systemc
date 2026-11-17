# Systemc
Model Mounting Verification Based on SystemC

## SPI 状态、中断与 DMA

| 位 | `SPI_SR` 状态 | `SPI_IER` 使能 | `SPI_DMAER` 使能 |
| --- | --- | --- | --- |
| 0 | TXE，TX FIFO 空 | TXE 中断 | TX 补数请求 |
| 1 | RXNE，RX FIFO 非空 | RXNE 中断 | RX 取数请求 |
| 2 | TC，帧传输完成 | TC 中断 | — |
| 3 | OVR，RX FIFO 溢出 | OVR 中断 | — |
| 4 | BUSY，正在传输 | — | — |
| 5 | TX FIFO 满 | — | — |
| 6 | RX FIFO 满 | — | — |

读取 `SPI_SR` 返回状态快照，并在同一次访问中清除快照里的 TC、OVR。
之后到达的新事件仍会锁存。OVR 在 FIFO 溢出信号上升时锁存，FIFO 本身将
该信号保持到 RX 被读空。

`irq`、`dma_tx_req` 和 `dma_rx_req` 均为高电平有效。使能的事件仍成立时，
对应输出保持有效。DMA 控制器用 `dma_tx_req` 触发写 `SPI_TXDR`，用
`dma_rx_req` 触发读 `SPI_RXDR`。两路输出可选连接，已有的 `dma_req`
仍输出两路请求的逻辑或。

每次 `SPI_TXDR` 写入立即占用一个 TX FIFO 槽，满时返回
`TLM_GENERIC_ERROR_RESPONSE`。每次 `SPI_RXDR` 读取立即取出一个 RX
FIFO 队首，空读返回 0。同一 delta 内的多次访问按调用顺序处理。FIFO 状态
信号和由此产生的 IRQ、DMA 请求随 SystemC delta 更新。

## 本地仿真

在 Windows PowerShell 下，使用 MinGW GCC 和安装于 `C:/systemc` 的
SystemC 3.0.2，在项目根目录运行以下命令。

```powershell
$sources = (Get-ChildItem -File -Filter 'spi_*.cpp').Name
$testExe = Join-Path $env:TEMP 'spi_status_irq_dma_test.exe'
g++ -std=c++17 -DSC_ALLOW_DEPRECATED_IEEE_API -Iheadfile -IC:/systemc/include $sources tests/test_spi_status_irq_dma.cpp -LC:/systemc/lib -lsystemc -lwinpthread -o $testExe
if ($LASTEXITCODE -eq 0) { & $testExe }
```

测试覆盖状态和使能逻辑、两路 DMA 信号、单帧回环，以及四种 SPI 模式下
通过 TLM 完成的八帧连续回环，还覆盖同一 delta 内读出八帧、重复读状态和
无效 TLM 请求。
