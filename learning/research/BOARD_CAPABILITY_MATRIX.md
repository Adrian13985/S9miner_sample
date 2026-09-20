# S9 XC7Z010 板卡能力矩阵

本表依据本地 15 页原理图、现有 Vivado 配置和公开 S9 设备树建立。它是课程选题边界，不是未经验证的板卡规格承诺。

## 已确认或高置信度资源

| 资源 | 板上路径/证据 | 可承载的学习内容 | 初始风险 |
|---|---|---|---|
| XC7Z010-1CLG400 | 四个工程目标器件 | PS/PL 架构、裸机、Linux、AXI 和 FPGA 逻辑 | 低 |
| PS 时钟 | 原理图页 3，33.33 MHz | PLL、PS 时钟树、外设分频 | 低；不修改时钟参数开始 |
| JTAG | 原理图页 2，2x7 Header | 下载、断点、寄存器与恢复 | 低 |
| PS UART1 | MIO48/49，原理图页 4/11 | 日志、CLI、轮询/中断、Linux 控制台 | 低 |
| PS LED | MIO15、MIO37、MIO38 | GPIO、电平有效性、状态机 | 低；先核对亮灭极性 |
| 按键 | MIO47、MIO51 | 输入、消抖、边沿中断 | 低；MIO47 同时与复位相关，先核对实际用途 |
| 蜂鸣器 | MIO39，经三极管 | GPIO/PWM/定时控制 | 中；限制占空比与连续响时长 |
| Micro SD | SD0 MIO40–46、MIO50 | 块设备、FatFs、BootROM/FSBL、Linux 根文件系统 | 低到中；先用可恢复 SD 卡 |
| NAND Flash | MIO0–14，MT29F2G08 | SMC/NAND、坏块、ECC、U-Boot/Linux MTD | 高；主线只读，禁止在无备份时擦写 |
| Ethernet | PS GEM0、RGMII、MDIO/MDC、B50612、RJ45 | PHY/MDIO、MAC DMA、lwIP、Linux 网络 | 中；PHY 配置和时钟需验证 |
| DDR3 | 原理图页 5/9，两片 x16 器件；现有 PS 配置为 32-bit、地址到 `0x1fffffff` | DDR、链接脚本、Cache、DMA、带宽测试 | 中；原理图器件标注、工程 preset 与实物容量存在核对任务 |
| PL LED | Bank35 四路 LED 网络 | RTL、XDC、计数器、PWM、ILA | 低；优先使用 PS FCLK，先确认板载 PL 晶振是否装配 |
| PS FCLK 到 PL | PS7 可配置输出 | PL 时钟、复位、AXI 时钟域 | 低 |
| 9 路算力板接口 | Bank34 PLUG/RST/TXD/RXD + MIO28–36 EN | 多通道 UART/串行状态机、调度、故障隔离 | 高；未接外设时先做内部回环/仿真 |
| 两组 PL I²C | Bank34 SCL1/SDA1、SCL2/SDA2 | 开漏、三态、仲裁、协议分析 | 中；确认上拉和连接对象后上板 |
| 6 路风扇测速 + 公共 PWM | Bank35，原理图页 14 | 脉冲计数、PWM、闭环控制、失速检测 | 高；涉及 12 V 负载与散热，不作为初学实验 |
| Board ID / CLOCK_OUT | Bank34/35 网络 | 输入同步、板卡识别、时钟输出 | 中；连接关系与方向需进一步核验 |

## 板卡特有的不确定项

### DDR 容量与器件型号

- 原理图页 9 的器件文字显示 `MT41K256M16HA-125:E`，按两片 x16 连接推导的容量与现有工程不一致。
- 四个 Vivado 工程配置为 `MT41K128M16 JT-125`、32-bit，并把高地址设为 `0x1fffffff`，即 512 MiB 地址范围。
- U-Boot 主线设备树声明 1 GiB，而部分社区启动日志又显示更小容量。

因此路线不预设实物容量。P18 必须用“读取配置 → 非破坏内存测试 → Linux/U-Boot 报告交叉验证”闭环确认，不能仅相信某一来源。

### PL 时钟

原理图页 6 标出两个 100 MHz 时钟位号，同时有 `NC` 标记。是否实际焊接必须看实物或测量。基础 RTL 实验默认使用 PS FCLK，经 Processor System Reset 产生同步复位；确认晶振后再做双时钟/CDC。

### Ethernet 速率

原理图是四对线 RGMII 连接，U-Boot 设备树使用 `rgmii-id` 和 PHY 地址 1；部分社区说明却称 Fast Ethernet。课程先完成 MDIO 识别和链路协商观测，再声明实际速率。

## 从其他开发板移植时的分类

| 类型 | 可以直接复用 | 必须重做 |
|---|---|---|
| 算法/状态机 | 纯 C 算法、与接口无关的 RTL、Testbench 思路 | 时钟假设、位宽、缓冲深度与错误处理 |
| PS 驱动示例 | 同版本 API 调用结构 | `xparameters.h` 标识、MIO、设备实例、中断号、时钟 |
| Vivado Block Design | IP 组合思路和 AXI 拓扑 | Part/Board preset、DDR、MIO、XDC、复位、地址 |
| Linux | 通用驱动框架、设备树 binding | `compatible`、`reg`、`interrupts`、`clocks`、PHY、存储分区 |
| DMA/高速数据 | 描述符、Cache 维护和测量方法 | HP 端口、DDR 地址、流宽、时钟、TLAST 和吞吐目标 |

## 安全分层

- **S0 安全入门**：UART、PS LED、PL LED、仿真、JTAG、使用新 SD 卡。
- **S1 受控实验**：按键、蜂鸣器、Ethernet、PL I²C、无外设的算力板接口内部回环。
- **S2 需要电气核验**：风扇、外接模块、算力板连接、时钟输出。
- **S3 需要备份与恢复**：NAND 擦写、启动区修改、远程更新。

任何实验从 S0 升到 S1/S2/S3 前，都要在实验卡上写明供电、方向、电压、恢复路径和停止条件。
