# Zynq-7000 完整能力地图

课程节点按“能解释并迁移的能力”组织，不按现有四个例程组织。`P01`–`P04` 只是其中四个锚点。

## 节点命名

- `T`：工具、工程与证据
- `C`：嵌入式 C、CPU 与内存模型
- `R`：RTL、验证、时钟与时序
- `P`：Zynq PS 裸机
- `B`：存储与启动
- `A`：PS–PL、AXI 与自定义 IP
- `M`：DDR、Stream、DMA 与性能
- `N`：通信与网络
- `O`：RTOS 与并发
- `L`：Linux 与驱动
- `S`：系统设计与本板专项

## T — 工具、工程与证据

| ID | 能力 | 关键产物/证据 | 先修 |
|---|---|---|---|
| T00 | 硬件识别与安全边界 | 实物版本、供电、跳线、串口、JTAG、恢复卡记录 | 无 |
| T01 | Vivado/SDK 2017.4 工程解剖 | 解释 XPR/BD/XDC/BIT/HDF/BSP/ELF/BIF/BOOT.BIN | T00 |
| T02 | 可复现构建 | 从源码和记录重建工程；区分手写与生成物 | T01 |
| T03 | 调试与观测 | UART、断点、寄存器、XSCT、ILA、示波器/逻辑分析仪的选择 | T01 |
| T04 | 实验方法 | 基线、单变量、预测、日志、故障注入和回退 | T00 |
| T05 | 版本与来源管理 | 文档版本、代码标签、许可证、移植差异清单 | T01 |

## C — 嵌入式 C、Cortex-A9 与内存模型

| ID | 能力 | 必须理解 | 先修 |
|---|---|---|---|
| C00 | 整数、位运算与表示 | 宽度、符号扩展、移位、掩码、字节序 | 无 |
| C01 | 指针、数组与容量 | 地址不携带长度、对象生命周期、越界、对齐 | C00 |
| C02 | 编译、链接与映像 | 段、符号、链接脚本、栈/堆、map 文件 | T01、C01 |
| C03 | `volatile` 与 MMIO | 编译器可见性、寄存器副作用、read-modify-write | C00 |
| C04 | 异常、原子性与并发 | ISR/主循环共享、临界区、内存屏障的边界 | C03 |
| C05 | Cache 与 DMA 可见性 | flush/invalidate、所有权、别名和缓冲区生命周期 | C01、C03 |
| C06 | 防御式驱动调用 | 返回值、超时、部分传输、资源释放、错误注入 | C01 |

## R — RTL、验证、时钟与时序

| ID | 能力 | 必须理解 | 先修 |
|---|---|---|---|
| R00 | 组合/时序逻辑 | LUT、触发器、阻塞/非阻塞赋值、锁存器风险 | T01 |
| R01 | 可综合 Verilog/SystemVerilog | 模块、参数、FSM、计数器、同步使能 | R00 |
| R02 | Testbench 与自检 | stimulus、scoreboard、断言、边界条件 | R01 |
| R03 | 综合与实现 | RTL→网表→布局布线→bitstream；资源报告 | R01、T01 |
| R04 | XDC 与 I/O | package pin、IOSTANDARD、时钟约束、电气方向 | R03、T00 |
| R05 | STA | setup/hold、时钟路径、slack、错误约束的后果 | R03、R04 |
| R06 | 复位设计 | 异步断言/同步释放、复位域、Processor System Reset | R01、R05 |
| R07 | CDC | 单比特同步、脉冲、握手、异步 FIFO、多位一致性 | R01、R05 |
| R08 | 存储与流控原语 | BRAM、ROM、同步 FIFO、满空、水位 | R02 |
| R09 | 片上调试 | ILA probe、触发、采样深度、不可观测性的代价 | T03、R03 |

## P — Zynq PS 裸机

| ID | 能力 | 本板锚点 | 先修 |
|---|---|---|---|
| P00 | PS/PL/互连与地址空间 | PS7 BD、`xparameters.h` | T01、C00 |
| P01 | 复位到 `main()` | `ps7_init.*`、startup、BSP、链接脚本 | C02、P00 |
| P02 | UART 轮询/FIFO/中断 | 现有 P01 | C03、P00 |
| P03 | PS GPIO 与 MIO | 现有 P02；MIO15/37/38/51 | C03、P00 |
| P04 | 时间基准 | Global Timer、SCU Timer、TTC、超时与漂移 | C00、P01 |
| P05 | GIC 与异常生命周期 | enable/pending/ack/EOI、优先级、触发类型 | C04、P01 |
| P06 | UART/GPIO/Timer 中断整合 | 非阻塞 CLI、按键事件、周期任务 | P02–P05 |
| P07 | 看门狗与故障恢复 | 超时、喂狗策略、复位原因 | P04、P05 |
| P08 | 双核基础 | 共享内存、AMP、Cache/中断风险 | C04、C05、P05 |

## B — 存储与启动

| ID | 能力 | 本板锚点 | 先修 |
|---|---|---|---|
| B00 | SD 控制器与块设备 | MIO40–46/50、XSdPs | C06、P01 |
| B01 | FatFs 与文件语义 | 现有 P03，`br/bw`、close、512 B sector | C01、C06、B00 |
| B02 | BootROM、启动脚与 OCM | JP1–JP4、UG585/UG1283 | P01、T00 |
| B03 | FSBL、BIF 与分区 | 现有 P04；FSBL/bit/ELF | B01、B02 |
| B04 | SD 冷启动与恢复 | 断电启动证据、失败回退卡 | B03 |
| B05 | NAND 只读识别 | SMC、ONFI/ID、坏块/ECC 概念 | B02、C06 |
| B06 | NAND/远程更新 | 分区、原子更新、回滚、校验 | B05、L03、S02；S3 风险 |

## A — PS–PL、AXI 与自定义 IP

| ID | 能力 | 必须理解 | 先修 |
|---|---|---|---|
| A00 | MIO/EMIO/PL I/O | 三条路径的资源、延迟、软件模型差异 | P03、R04 |
| A01 | AXI4-Lite 与 MMIO | 五通道握手、独立通道、寄存器契约 | P00、R01、C03 |
| A02 | AXI GPIO/Timer | Address Editor、驱动、PL IRQ | A01、P05 |
| A03 | 自定义 AXI-Lite 从设备 | RW/RO/W1C、byte strobe、错误/非法访问 | A01、R02 |
| A04 | PL→PS 中断 | 同步、脉冲保持、GIC、清中断顺序 | A02、R06、R07 |
| A05 | BRAM 共享 | AXI BRAM、双口、所有权、吞吐/一致性 | A01、R08 |
| A06 | 寄存器 ABI 与驱动层 | 版本、能力寄存器、封装、测试 | A03、C06 |

## M — DDR、AXI-Stream、DMA 与性能

| ID | 能力 | 必须理解 | 先修 |
|---|---|---|---|
| M00 | DDR 配置与内存测试 | 容量、宽度、训练、地址范围；本板资料冲突核验 | P01、C02 |
| M01 | AXI4 与 HP/ACP/GP | master/slave、burst、outstanding、端口选择 | A01、P00 |
| M02 | AXI4-Stream | ready/valid、backpressure、TLAST、packet boundary | R08、A01 |
| M03 | Stream FIFO/loopback | 停顿、溢出、丢包和 ILA 观测 | M02、R09 |
| M04 | AXI DMA simple polling | MM2S/S2MM、buffer、Cache、timeout | M01–M03、C05 |
| M05 | AXI DMA interrupt | 完成/错误中断、所有权状态机 | M04、A04 |
| M06 | Scatter/Gather | descriptor ring、批处理、环形缓冲 | M05 |
| M07 | 性能建模与测量 | 理论带宽、有效吞吐、延迟、CPU 占用、瓶颈定位 | M04、T04 |
| M08 | PL master 访问 DDR | burst 生成、数据校验、背压、仲裁 | M01、M03、M07 |

## N — 通信与网络

| ID | 能力 | 本板锚点 | 先修 |
|---|---|---|---|
| N00 | 串行协议通用模型 | frame、clocking、framing/error、timeout | P02、C06 |
| N01 | I²C | 开漏、上拉、START/STOP、ACK、仲裁、PL 三态 | N00、R07 |
| N02 | SPI | mode、CS、full-duplex、transaction boundary | N00 |
| N03 | 多通道 UART/自定义串行 | 9 路 TX/RX，先仿真和内部回环 | N00、R02、R08 |
| N04 | MDIO 与 PHY bring-up | PHY 地址、寄存器、协商、RGMII delay | P03、C06 |
| N05 | PS GEM 与 lwIP | descriptor、DMA、Cache、zero-copy、TCP/UDP | N04、C05、P05 |
| N06 | 网络化数据通路 | DMA/缓冲/协议/丢包/流控与测量 | N05、M07 |

## O — RTOS 与并发

| ID | 能力 | 必须理解 | 先修 |
|---|---|---|---|
| O00 | 调度与任务 | tick、优先级、阻塞态、stack | P04、C04 |
| O01 | Queue/Semaphore/Event | 所有权、同步、ISR-safe API | O00、P05 |
| O02 | RTOS 外设架构 | ISR→queue→worker、超时、反压 | O01、P06 |
| O03 | 实时分析 | WCET、抖动、优先级反转、负载与看门狗 | O02、P07 |

## L — Linux 与驱动

| ID | 能力 | 必须理解 | 先修 |
|---|---|---|---|
| L00 | U-Boot/Linux 启动链 | FSBL→U-Boot→kernel→DTB→rootfs | B04 |
| L01 | 设备树 | compatible/reg/interrupts/clocks/pinctrl/phy | P00、A01、L00 |
| L02 | 用户空间硬件访问 | sysfs/libgpiod、UIO、`/dev/mem` 风险 | L01、A06 |
| L03 | Kernel module 与字符设备 | probe/open/read/write/ioctl、生命周期 | C04、L01 |
| L04 | platform driver | OF match、资源、devm、并发 | L03 |
| L05 | 中断/等待队列/poll | top/bottom half、阻塞/nonblocking/async I/O | L04、P05 |
| L06 | Linux I²C/SPI/PWM/GPIO 子系统 | 不重复造轮子，理解框架和 binding | L04、N01、N02 |
| L07 | Linux DMA/Cache/IOMMU 边界 | DMA API、coherent/streaming mapping | L04、M04 |
| L08 | 网络与板级服务 | GEM/PHY、网络配置、日志、更新与守护 | L01、N05 |
| L09 | 可复现 Linux 构建 | PetaLinux/源码树、版本锁定、镜像清单 | T02、T05、L00 |

## S — 系统设计与本板专项

| ID | 能力 | 验收 | 先修 |
|---|---|---|---|
| S00 | 需求与 PS/PL 划分 | 用吞吐、时延、确定性、资源和维护成本论证 | M07、O03、L04 |
| S01 | 板级诊断程序 | 自动检测 LED/键/SD/DDR/PHY，输出可审计报告 | P06、B01、M00、N04 |
| S02 | 故障、恢复与更新 | brownout/卡损坏/进程崩溃/镜像损坏的恢复设计 | P07、B04、L09 |
| S03 | 风扇测速与闭环 PWM | tach 捕获、滤波、控制、失速/过温保护 | R07、A04、O03；S2 风险 |
| S04 | 多通道外设控制平面 | 9 通道状态机、队列、超时、隔离和遥测 | N03、O02、M07；S2 风险 |
| S05 | 数据采集/网络管线 | PL→DMA→DDR→网络，含测量和丢包策略 | M05、N06 |
| S06 | 综合毕业项目 | 设计说明、可复现构建、故障矩阵、无提示答辩和远迁移 | S00–S05 中所需节点 |

## 依赖主干

```text
T00 -> T01 -> T02/T03/T05
          |       |
          v       v
C00 -> C01 -> C02/C03 -> P00 -> P01 -> P02/P03/P04 -> P05 -> P06
                    |                         |                 |
                    v                         v                 v
R00 -> R01 -> R02 -> R03 -> R04/R05 -> R06/R07 -> A00 -> A01 -> A02/A03 -> A04
                                 |                           |
                                 v                           v
                                R08 -> M02 -> M03 -> M04 -> M05 -> M06/M07

P01 -> B00 -> B01 -> B02 -> B03 -> B04 -> L00 -> L01 -> L03/L04 -> L05/L07
P03 -> N04 -> N05 -> N06
P04/P05 -> O00 -> O01 -> O02 -> O03

A/M/N/O/L 各主干 -> S00 -> S01..S05 -> S06
```

## 每个节点的完成标准

每个节点必须留下六类内容：

1. 最小因果模型；
2. 权威来源与版本；
3. 一个安全、可复现的实验；
4. 一个故障注入和定位记录；
5. 一个改变约束的迁移任务；
6. 一次延迟后的无提示复测。

掌握等级仍使用 `MASTERY.md` 的 L0–L4。跟做一遍或让 AI 写出代码最多支持 L1；只有独立重建和迁移证据才能进入 L2/L3。
