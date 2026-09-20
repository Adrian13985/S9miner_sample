# 在线资料与代码目录

调研日期：2026-09-21。这个目录记录“在哪里取证、拿什么来学、怎样迁移”，不把网页数量当作课程完整度。

## 采用等级

| 等级 | 含义 | 使用方式 |
|---|---|---|
| A | 本板资料，或与 Vivado/SDK 2017.4 直接匹配 | 可以作为复现实验的首选输入，仍需核对硬件版本 |
| B | 同一 Zynq-7000 架构或同一 XC7Z010-CLG400 器件 | 用于移植设计；重新做 PS 配置、XDC、时钟和外设映射 |
| C | 更新工具版本或不同板卡上的完整教程 | 采用知识顺序和设计方法，不照抄界面步骤、地址或管脚 |
| D | 社区项目 | 用于发现可行方向和故障经验；关键事实回到原理图、AMD 文档和实测 |

## 1. 本板与同板项目

| 等级 | 资料/代码 | 能解决的问题 | 约束 |
|---|---|---|---|
| A | [KarolNi/S9miner_sample](https://github.com/KarolNi/S9miner_sample) | 当前四个工程和原理图的上游来源 | 只有一次提交、四个入门例程，不能充当完整课程 |
| A | 本地 `AntMiner_ControlBoard_XC7010_V1.01控制板.pdf` | 启动跳线、MIO、PL 管脚、DDR、NAND、SD、UART、以太网、风扇和算力板接口 | 原理图描述的是板级连接，不证明元件一定装配或工程配置正确 |
| A/B | [Xilinx U-Boot 的 Antminer S9 设备树](https://github.com/Xilinx/u-boot-xlnx/blob/master/arch/arm/dts/bitmain-antminer-s9.dts) | Linux 板级模型、UART1、SD0、GEM0、NAND、看门狗和 PHY 地址 | 当前 `master` 不是 2017.4；移植时要选定 U-Boot/Linux 版本 |
| B | [polprog/antminer_zynq](https://github.com/polprog/antminer_zynq) | S9 启动日志、设备树、XSA 与 PetaLinux 方向 | 社区项目；逐文件核对许可证和硬件变体 |
| B/D | [Muhammad-Yunus/Antminer-S9-PYNQ](https://github.com/Muhammad-Yunus/Antminer-S9-PYNQ) | GPIO、I²C、SPI、I²S、DVI、摄像头和自定义 Overlay 的迁移样例 | PYNQ 2.5 路线，不替代裸机、RTL、时序与 Linux 驱动基础 |
| D | [guannan-he/Antminer_s9_pynq](https://github.com/guannan-he/Antminer_s9_pynq) | S9 PYNQ 移植线索 | 维护状态和许可证需在实际取用前再次核验 |

## 2. 用来拟定路线的完整课程

| 等级 | 课程 | 覆盖 | 本项目采用方式 |
|---|---|---|---|
| C | [ALINX AX7010 全套课程与工程](https://github.com/alinxalinx/AX7010_2023.1) / [在线目录](https://ax7010-20231-v101.readthedocs.io/zh-cn/latest/) | 同为 XC7Z010-1CLG400；RTL、仿真、FIFO、PWM、I²C、裸机、MIO/EMIO、AXI GPIO、自定义 IP、GIC、lwIP、FreeRTOS、BRAM、DDR、DMA、HLS、PetaLinux、驱动 | 作为“覆盖面母版”；不复制 2023.1 的 Vitis 操作，不复制 AX7010 管脚 |
| B/C | [The Zynq Book](https://www.zynqbook.com/) / [Tutorials PDF](https://www.zynqbook.com/downloads/The_Zynq_Book_Tutorials_Aug_15.pdf) | 2015.1 SDK 时代的 PS+PL、IP Integrator、中断、自定义 IP、HLS 与综合音频系统 | 采用从小系统到集成系统的教学递进；工具年代接近本项目，但板卡不同 |
| C | [AMD Zynq-7000 Embedded Design Tutorial UG1165](https://docs.amd.com/r/2023.2-English/ug1165-zynq-embedded-design-tutorial) | PS-only、调试、Linux、GP/HP、AXI CDMA、启动、自定义 AXI IP、Linux 驱动 | 作为官方端到端流程清单；新版 Vitis 界面步骤需转换为 2017.4 SDK 流程 |
| B/C | [Digilent Zybo Z7 官方演示仓库](https://github.com/Digilent/Zybo-Z7) | DMA 音频、HDMI、XADC、PetaLinux 和可复现硬件/软件交付结构 | Zybo Z7-10 与本板同为 XC7Z010-1CLG400；采用工程组织与 DMA/Linux 思路，全部重做外设映射 |
| B | [Zybo Z7-10 Base Linux，Vivado 2017.2.1](https://github.com/Digilent/Zybo-Z7-10-base-linux) / [PetaLinux 2017.4](https://github.com/Digilent/Petalinux-Zybo-Z7-10) | 与当前工具年代接近的 Linux 硬件平台、时钟、约束和 PetaLinux BSP | 用于理解 2017.x 交付物与时钟问题；不移植 Zybo XDC |

## 3. 架构、工具与 IP 的权威资料

### Zynq 与软件

- [UG585 Zynq-7000 Technical Reference Manual](https://docs.amd.com/r/en-US/ug585-zynq-7000-SoC-TRM)：PS、地址空间、DDR、GPIO、UART、SD、GEM、GIC、定时器和启动寄存器的主依据。
- [UG821 Zynq-7000 Software Developers Guide](https://docs.amd.com/r/en-US/ug821-zynq-7000-swdev)：启动到应用、BSP、调试和软件结构。
- [UG1283 Bootgen User Guide 的 Zynq-7000 启动章节](https://docs.amd.com/r/en-US/ug1283-bootgen-user-guide/Zynq-7000-SoC-Boot-and-Configuration)：BootROM、FSBL、BIF 和镜像分区。
- [Zynq-7000 2017.4 发布信息](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842330/Zynq2017.4%2BRelease)：Linux、U-Boot、设备树等 2017.4 对应标签。

### FPGA、时序与 AXI

- [UG900 Logic Simulation](https://docs.amd.com/r/en-US/ug900-vivado-logic-simulation)、[UG901 Synthesis](https://docs.amd.com/r/en-US/ug901-vivado-synthesis)、[UG903 Constraints](https://docs.amd.com/r/en-US/ug903-vivado-using-constraints/Timing-Constraints)、[UG906 Design Analysis](https://docs.amd.com/r/en-US/ug906-vivado-design-analysis)：仿真、综合、XDC、STA 和收敛。
- [UG1037 Vivado AXI Reference Guide](https://docs.amd.com/v/u/en-US/ug1037-vivado-axi-reference-guide)：2017 年版，区分 AXI4、AXI4-Lite、AXI4-Stream。
- [PG079 AXI Timer](https://docs.amd.com/r/en-US/pg079-axi-timer/Operation-Overview) 与 [PG021 AXI DMA](https://docs.amd.com/r/en-US/pg021_axi_dma/Core-Overview)：定时器、PWM、MM2S/S2MM、简单 DMA 与 SG DMA。

### 文件系统

- [FatFs 官方站点](https://elm-chan.org/fsw/ff/)、[应用说明](https://elm-chan.org/fsw/ff/doc/appnote.html) 和 [`f_write`](https://elm-chan.org/fsw/ff/doc/write.html)：文件 API 的字节数、返回值和块设备抽象。

## 4. 版本锁定的官方代码入口

裸机阶段优先从 [`Xilinx/embeddedsw` 的 `xilinx-v2017.4` 标签](https://github.com/Xilinx/embeddedsw/tree/xilinx-v2017.4) 取参考代码，不从 `master` 直接复制到 SDK 2017.4。

| 主题 | 2017.4 目录或示例 | 学习目的 |
|---|---|---|
| PS UART | `XilinxProcessorIPLib/drivers/uartps/examples/` | 比较轮询、低级寄存器、回显和中断模型 |
| PS GPIO | `XilinxProcessorIPLib/drivers/gpiops/examples/` | 管脚/Bank、方向、输出使能、边沿中断 |
| GIC | `XilinxProcessorIPLib/drivers/scugic/examples/` | 异常入口、分发器、CPU 接口和 EOI 生命周期 |
| SCU Timer/TTC | `drivers/scutimer/examples/`、`drivers/ttcps/examples/` | 周期时基、回调和中断路由 |
| SD | `drivers/sdps/examples/` | SD 控制器层；与 FatFs 层分开理解 |
| FatFs | `lib/sw_services/xilffs/` | FatFs 与 Xilinx `diskio` 胶水层 |
| Ethernet | `drivers/emacps/` 与同标签 lwIP 组件 | PHY/MDIO、MAC、描述符、Cache 与协议栈 |
| AXI GPIO/Timer | `drivers/gpio/examples/`、`drivers/tmrctr/examples/` | MMIO 外设和 PL 中断 |
| AXI DMA | `drivers/axidma/examples/` | polling/interrupt、simple/SG、Cache 维护 |
| FSBL | `lib/sw_apps/zynq_fsbl/` | BootROM 之后的初始化、分区装载和跳转 |

Linux 阶段使用同一发行线的代码：

- [`linux-xlnx` `xilinx-v2017.4`](https://github.com/Xilinx/linux-xlnx/tree/xilinx-v2017.4)
- [`u-boot-xlnx` `xilinx-v2017.4`](https://github.com/Xilinx/u-boot-xlnx/tree/xilinx-v2017.4)
- [`device-tree-xlnx` `xilinx-v2017.4`](https://github.com/Xilinx/device-tree-xlnx/tree/xilinx-v2017.4)

## 5. 收集规则

1. 当前阶段只保存链接、版本、用途、适配差异和许可证状态，不盲目把大型仓库复制进项目。
2. 真正开始某个实验时，只取该实验所需的最小源码，并保留原路径、标签/提交和许可证声明。
3. 没有清晰许可证的课程代码只能用于阅读和独立重写，不能直接并入本项目。
4. 任何其他板卡工程都必须重新核对：器件、PS preset、DDR 参数、MIO、PL XDC、电压、时钟、复位和外设芯片。
5. `master` 代码只用于理解最新修正；SDK 2017.4 的可编译基线以 `xilinx-v2017.4` 为准。
6. 每个项目在 `PROJECT_BACKLOG.md` 中明确“参考来源”和“必须独立完成的部分”，防止再次退化为照抄例程。
