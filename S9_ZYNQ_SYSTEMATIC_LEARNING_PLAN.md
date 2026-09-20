# 蚂蚁 S9 控制板（Zynq-7010）系统学习教程

> 适用硬件：蚂蚁 S9 控制板、XC7Z010-1CLG400  
> 适用软件：Vivado 2017.4、Xilinx SDK 2017.4  
> 当前基础：接触过 STM32、C 语言和寄存器配置，但需要系统复习；已经能够通过 14 Pin JTAG 识别芯片、下载 bitstream 和 ELF，并通过 UART 运行串口命令程序

## 1. 学习目标

完成这套教程后，应能够：

- 理解 Zynq 的 PS、PL 和 AXI 三部分及其关系。
- 使用 Vivado 2017.4 创建、综合、实现和下载硬件设计。
- 使用 SDK 编写并调试 Cortex-A9 裸机程序。
- 控制板载 LED、读取按键、使用定时器和中断。
- 使用 FATFS 在 SD 卡上读写文件。
- 制作 `BOOT.BIN`，让板子脱离 JTAG 从 SD 卡启动。
- 编写、仿真并下载简单的 Verilog 逻辑。
- 使用 AXI GPIO 和自定义 AXI4-Lite IP 实现 PS/PL 协同。
- 使用 ILA 分析 PL 内部信号。
- 为后续 DMA、U-Boot、Linux、设备树和驱动开发打下基础。

## 2. Zynq 的基本结构

### 2.1 从 STM32 迁移到 Zynq 的核心映射

这套教程采用“双线学习”：在每个 Zynq 实验中复习对应的 STM32/C 基础，再学习 Zynq 的实现方式。已经掌握的内容可以快速通过，不熟悉的内容则在实际代码和寄存器路径中补齐，避免先脱离硬件重新学习一整套抽象理论。

| STM32 中的概念 | Zynq 中的对应概念 | 关键差异 |
|---|---|---|
| Cortex-M 内核 | PS 中的双核 Cortex-A9 | Cortex-A 带 MMU、Cache，可运行 Linux，异常体系也不同 |
| CubeMX / RCC 时钟树 | Vivado Zynq PS 配置、`ps7_init`、FSBL | PS 时钟和 MIO 配置属于硬件平台，通常不是在 `main()` 中逐项配置 |
| GPIO 复用与 AF | MIO、EMIO、PL I/O、XDC | MIO 直连 PS；EMIO 经过 PL；PL 管脚还要配置封装管脚和 Bank 电压 |
| HAL/LL 或 CMSIS 驱动 | Xilinx BSP 驱动，如 `XGpioPs`、`XUartPs` | 驱动实例通常先 `LookupConfig`，再 `CfgInitialize` |
| `GPIOA->ODR` 等寄存器访问 | `Xil_In32()`、`Xil_Out32()` 或驱动 API | 外设基地址来自 `xparameters.h`，PL 外设地址由 Address Editor 分配 |
| AHB/APB 外设总线 | AXI4-Lite、AXI Memory Mapped、AXI-Stream | AXI 不只是片内固定总线，也连接用户在 PL 中创建的硬件 |
| NVIC | GIC（Generic Interrupt Controller） | 需要同时配置异常入口、GIC 和具体外设中断源 |
| SysTick / TIM | SCU Timer、TTC、私有定时器或 PL 定时器 | 中断号和时钟来源由 PS 配置及 `xparameters.h` 决定 |
| STM32 DMA | PS DMA、AXI DMA | DDR 与 Cache 并存时必须处理 flush/invalidate 和内存一致性 |
| 启动文件 + `SystemInit()` | BootROM + FSBL + `ps7_init` + 应用 ELF | FSBL 还可初始化 DDR、配置 PL 并加载后续镜像 |
| 链接脚本 `.ld` | SDK 工程中的 `lscript.ld` | 代码和数据常放在 DDR，不再只关注片上 Flash/SRAM |
| ST-Link / SWD | Xilinx JTAG、Hardware Server、SDK Debug | bitstream 配置 PL，ELF 在 PS 上运行，两者不是同一种产物 |
| FatFs | Xilinx BSP 中的 FATFS | FatFs API 基本可迁移，底层磁盘驱动和缓存行为不同 |

迁移时可以使用下面的思考顺序：

```text
先判断功能属于 PS 还是 PL
→ 如果属于 PS：寻找 BSP 驱动、基地址和中断号
→ 如果属于 PL：先完成 RTL/IP、时钟、复位、约束和地址映射
→ 再决定使用轮询、中断还是 DMA
→ 最后处理 Cache、并发和启动方式
```

建议为每个实验都画两条路径：

```text
软件路径：C 变量/函数 → BSP 驱动 → 寄存器地址 → PS 外设
硬件路径：外部引脚/PL 信号 → FIFO或状态位 → AXI/中断 → Cortex-A9
```

遇到不熟悉的知识点时，按“先看现象 → 再看 API → 进入驱动源码 → 查寄存器手册”的顺序学习，不要求一开始背完所有寄存器。

### 2.2 与 STM32 相比最需要改变的习惯

1. **不要把 bitstream 和 ELF 混为一谈。** bitstream 定义 PL 硬件，ELF 是 Cortex-A9 软件；只改 C 一般无需重新生成 bitstream。
2. **不要默认所有引脚都由 GPIO 寄存器控制。** 先确认它属于 MIO、EMIO 还是 PL I/O，再决定使用 `XGpioPs`、`XGpio` 还是 Verilog。
3. **不要默认地址固定。** PS 外设地址相对固定，PL AXI 外设地址由 Vivado Address Editor 分配，并通过 HDF/BSP 进入 `xparameters.h`。
4. **不要忽略 Cache。** GPIO、UART 等设备寄存器按设备内存访问；DMA 缓冲区则必须显式处理 D-Cache 一致性。
5. **不要用软件循环代替 PL。** 高精度并行时序、连续高速采样、协议波形和多通道同步更适合在 PL 中实现。
6. **不要把 Verilog 当成顺序执行的 C。** `always` 块和连续赋值表示并行硬件，时序逻辑必须围绕时钟、复位和时序约束思考。

### 2.3 随实验复习的 STM32/C 知识清单

| Zynq 学习阶段 | 同步复习的 STM32/C 知识 | 需要达到的程度 |
|---|---|---|
| P01 串口 | 整数类型、字符与 ASCII、数组、指针、函数、`switch`、UART 收发 | 能解释一个字符怎样从寄存器进入变量 |
| P02 GPIO | 位运算、输入输出模式、上拉下拉、有效电平、按键消抖 | 能从原理图和寄存器判断 LED/按键状态 |
| Timer/GIC | 时基、计数器、预分频、中断使能、挂起、清除、`volatile` | 能写短 ISR，并让主循环处理业务 |
| SD/FatFs | 块设备、缓冲区、数组越界、返回值、文件指针 | 能安全读写指定长度并处理错误 |
| FSBL/启动 | 向量表、Reset Handler、链接脚本、Flash/RAM 初始化 | 能解释上电到 `main()` 的完整路径 |
| Verilog/PL | 组合逻辑、触发器、时钟、复位、状态机 | 能区分软件顺序执行与硬件并行工作 |
| AXI4-Lite | 基地址、偏移、位域、读写属性、`volatile`/MMIO | 能设计寄存器表并封装 C 驱动 |
| 中断/ILA | 边沿与电平、同步、竞争、观测点 | 能定位中断重复或信号未到达的问题 |
| AXI DMA | DMA 方向、宽度、长度、完成中断、环形缓冲 | 能额外处理 DDR Cache 一致性 |

每项只要求达到“能够结合当前实验解释和使用”，不要求脱离项目背诵全部 STM32 型号的寄存器。STM32F1 与 F4/H7 等系列的 GPIO 寄存器名称可能不同，但输入、输出、复用、上下拉和数据寄存器等核心概念相同。

### 2.4 PS、PL 和 AXI

XC7Z010 不是单纯的 FPGA，它由三部分共同构成：

| 部分 | 全称 | 主要资源 | 主要开发工具 |
|---|---|---|---|
| PS | Processing System | 双核 Cortex-A9、DDR、UART、GPIO、SD、以太网 | SDK、C 语言 |
| PL | Programmable Logic | LUT、触发器、BRAM、DSP、可编程 I/O | Vivado、Verilog |
| AXI | Advanced eXtensible Interface | PS 与 PL 之间的控制和数据通道 | Vivado Block Design、C 驱动 |

可以先把 **PS 理解为一颗带 DDR、Cache 和 GIC 的高性能 STM32**，把 **PL 理解为可自行设计外设甚至数据通路的数字逻辑区域**。AXI 则承担两者之间的寄存器访问和高速数据传输。

### 2.5 当前 Hello World 已验证的路径

当前的 Hello World 主要经过以下路径：

```text
Cortex-A9 → PS UART1 → 板载串口 → USB-TTL → 电脑串口终端
```

因此，Hello World 跑通主要证明了以下部分正常：

- JTAG 连接正常。
- PS 初始化脚本可以执行。
- Cortex-A9 可以停止、复位和运行。
- DDR 可以装载并执行 ELF。
- PS UART1 与电脑串口连接正常。

它还没有证明自定义 PL 逻辑、AXI 外设和 DMA 已经掌握。

## 3. 已知板卡资源

板卡原理图：[`AntMiner_ControlBoard_XC7010_V1.01控制板.pdf`](./AntMiner_ControlBoard_XC7010_V1.01控制板.pdf)

根据当前示例工程，可以先建立以下资源表：

| 资源 | 工程中的定义 |
|---|---|
| Zynq 芯片 | XC7Z010-1CLG400 |
| 调试串口 | PS UART1 |
| 串口管脚 | MIO48、MIO49 |
| 串口参数 | 115200、8 数据位、无校验、1 停止位 |
| 红色 LED | MIO37 |
| 绿色 LED | MIO38 |
| 另一个 LED | MIO15 |
| 按键 S1 | MIO47 |
| 按键 S2 | MIO51 |
| SD 卡 | PS SD 控制器 |
| 调试接口 | Xilinx 14 Pin JTAG |

这些定义可以在 [`P02_GPIO/GPIO.sdk/GPIO/src/main.c`](./P02_GPIO/GPIO.sdk/GPIO/src/main.c) 中核对。

每开发一个新接口，都应在自己的笔记中记录：

- 原理图网络名。
- Zynq MIO、EMIO 或 PL 管脚号。
- 所属 I/O Bank 和 VCCO 电压。
- 输入、输出或双向。
- 高电平有效还是低电平有效。
- 是否有上拉、下拉、缓冲器或电平转换器。
- 是否与启动模式、NAND、SD 或其他外设复用。

## 4. 文件和下载方式

### 4.1 常见文件

| 文件 | 作用 |
|---|---|
| `.xpr` | Vivado 工程 |
| `.bd` | Block Design |
| `.v` | Verilog 源文件 |
| `.xdc` | PL 管脚和时序约束 |
| `.bit` | PL 配置数据 |
| `.hdf` | 导出给 SDK 的硬件描述，2017.4 使用此格式 |
| `.elf` | ARM 可执行程序 |
| `FSBL.elf` | 第一阶段启动程序 |
| `.bif` | 启动镜像内容与排列说明 |
| `BOOT.BIN` | SD、QSPI 或 NAND 启动时由 BootROM 读取的镜像 |

### 4.2 JTAG 下载是易失的

通过 JTAG 下载的内容在断电后会消失：

- `.bit` 只临时配置 PL。
- `.elf` 通常下载到 DDR 中运行。
- 重新上电后，需要重新初始化和下载。

### 4.3 日常开发流程

仅修改 C 程序时：

```text
修改 C 文件
→ Build Project
→ Run As / Debug As
→ Launch on Hardware
```

如果没有断电，而且硬件设计没有变化，通常不需要重新生成 bitstream。

修改 PS 配置、Block Design、IP 或 PL 逻辑时：

```text
Validate Design
→ Generate Output Products
→ Create/Update HDL Wrapper
→ Generate Bitstream
→ Export Hardware（包含 bitstream）
→ SDK 更新硬件平台或 BSP
→ Program FPGA
→ 下载并运行 ELF
```

板子断电重启且处于 JTAG 启动模式后：

```text
Program FPGA
→ SDK 下载并运行 ELF
```

## 5. 总体学习路线

```text
阶段一：串口与裸机 C 程序
        ↓
阶段二：PS GPIO、定时器和中断
        ↓
阶段三：SD 卡 FATFS
        ↓
阶段四：SD 卡独立启动
        ↓
阶段五：Verilog、仿真和 PL 管脚
        ↓
阶段六：AXI GPIO 和自定义 AXI IP
        ↓
阶段七：ILA、中断和 DMA
        ↓
阶段八：U-Boot、Linux、设备树和驱动
```

## 6. 阶段一：P01 串口和裸机 C 程序

### 6.1 目标

- 掌握 SDK 工程结构和编译流程。
- 理解 BSP、ELF、链接脚本和标准输入输出。
- 能够通过串口打印和接收字符。
- 能够使用断点、单步和变量观察。

STM32 迁移提示：本阶段不做脱离工程的语法题，而是在串口程序中复习字符、指针、函数、分支和状态变量。同时观察 `platform.c`、`lscript.ld`、BSP、`xparameters.h` 和 ELF 下载流程分别对应 STM32 工程中的哪些部分。

### 6.2 实验一：循环打印

将 Hello World 修改为：

```c
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"

int main()
{
    unsigned int count = 0;

    init_platform();

    while (1)
    {
        xil_printf("Hello Zynq, count = %u\r\n", count++);
        sleep(1);
    }

    cleanup_platform();
    return 0;
}
```

验收条件：

- 串口每秒输出一行。
- 计数值连续增加。
- 重新下载 ELF 后计数从 0 开始。
- 只修改 C 程序时能够不重新生成 bitstream。



### 6.3 实验二：串口命令

#### 6.3.1 实验目标

实现以下命令：

| 输入 | 功能 |
|---|---|
| `h` | 显示帮助信息 |
| `i` | 显示当前计数值 |
| `r` | 将计数清零 |
| `q` | 暂停周期打印 |
| `c` | 继续周期打印 |

学习重点：

- `inbyte()` 和 `outbyte()`。
- 阻塞接收与非阻塞接收的区别。
- 字符、字符串和数字转换。
- 状态变量和命令解析。

本实验同时复习 `switch`、字符、指针和状态变量，但最终要把这些 C 语言知识落实到真实硬件路径，并验证以下三件事：

- `STDIN_BASEADDRESS` 如何从 BSP 生成的 `xparameters.h` 获得。
- `inbyte()` 最终如何访问 PS UART 寄存器。
- 为什么只重新编译和下载 ELF 就能修改软件行为，而不需要重新生成 bitstream。

#### 6.3.2 本实验需要复习的 C 语言基础

1. **字符和 ASCII**：`char command` 保存的是一个字节；`'h'` 是字符常量，对应 ASCII 数值 `0x68`，它与字符串 `"h"` 不同。
2. **指针和取地址**：调用 `handle_command(command, &count, &periodic_print_enabled)` 时传入变量地址，函数中用 `*count` 和 `*periodic_print_enabled` 修改主函数中的原变量。
3. **`switch-case`**：适合处理有限个离散命令；两个连续的 `case 'h': case 'H':` 共用同一段代码。
4. **自增运算**：`count++` 先使用当前值再自增，因此打印 0 后变量已经变为 1。
5. **作用域**：`command` 只在接收循环内部有效；`count` 和状态变量属于 `main()` 的局部变量，通过指针传给处理函数。
6. **宏和单位**：`10000U` 中的 `U` 表示无符号整数；代码明确区分微秒 `US` 和毫秒 `MS`。
7. **状态变量**：`periodic_print_enabled` 不直接完成打印，而是记录程序当前处于“运行”还是“暂停”状态。

建议先独立写出并预测下面代码的结果：

```c
unsigned int count = 0U;
unsigned int *p = &count;

xil_printf("%u\r\n", count++); /* 输出 0，随后 count 变成 1 */
*p = 5U;                        /* 通过指针把 count 改成 5 */
```

#### 6.3.3 STM32 UART 寄存器知识复习

无论使用 STM32 还是 Zynq，轮询接收的底层逻辑都是：

```text
读取状态寄存器
→ 判断接收 FIFO/数据寄存器是否有数据
→ 有数据才读取
→ 将读出的低 8 位解释为字符
```

典型 STM32 写法可以抽象为：

```c
if ((USARTx->SR & USART_SR_RXNE) != 0U)
{
    command = (char)USARTx->DR;
}
```

不同 STM32 系列可能使用 `ISR/RDR` 而不是 `SR/DR`，HAL 中则可能使用 `HAL_UART_Receive()`。寄存器名字不同，但“状态位 + 数据寄存器/FIFO”的结构不变。

当前 Zynq 工程使用 PS UART1：

| 项目 | 数值或含义 |
|---|---|
| UART1 基地址 | `0xE0001000` |
| 状态寄存器偏移 | `0x002C` |
| FIFO 寄存器偏移 | `0x0030` |
| `RXEMPTY` | bit 1；为 1 表示 FIFO 为空 |
| `TXFULL` | bit 4；为 1 表示发送 FIFO 已满 |

`XUartPs_IsReceiveData()` 检查 `RXEMPTY`；`inbyte()` 最终调用 `XUartPs_RecvByte()` 读取 FIFO。注意 `inbyte()` 会一直等到收到字符，所以必须先检查是否有数据，才能让主循环继续执行其他任务。

与 STM32 的 `RXNE` 相比，Zynq 的 `RXEMPTY` 判断方向相反：STM32 常见的 `RXNE=1` 表示有数据，Zynq 的 `RXEMPTY=0` 才表示有数据。

#### 6.3.4 阻塞、轮询和中断

| 方式 | 行为 | 本实验是否使用 |
|---|---|---|
| 阻塞接收 | 没有字符时停在接收函数中等待 | `inbyte()` 本身具有此特性，但代码避免空调用 |
| 非阻塞轮询 | 主循环先检查状态，有数据才读取 | 是 |
| 中断接收 | UART 收到数据后由 GIC 打断主程序，ISR 搬运数据 | 否，后续中断阶段学习 |

本实验每 10 ms 检查一次 UART，最坏命令响应延迟约为一个轮询周期再加当前打印时间。它仍包含短暂的 `usleep()`，所以不是严格意义的完全异步程序，但已经解决 `sleep(1)` 导致命令长时间无响应的问题。

#### 6.3.5 完整参考代码

```c
#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xparameters.h"
#include "xuartps_hw.h"

#define POLL_INTERVAL_US  10000U
#define PRINT_INTERVAL_MS 1000U

static void print_help(void)
{
    xil_printf("Commands:\r\n");
    xil_printf("  h - show this help\r\n");
    xil_printf("  i - show the current count\r\n");
    xil_printf("  r - reset the count to zero\r\n");
    xil_printf("  q - pause periodic printing\r\n");
    xil_printf("  c - continue periodic printing\r\n");
}

static void handle_command(char command, unsigned int *count,
                           int *periodic_print_enabled)
{
    /* Echo the received command so the use of outbyte() is explicit. */
    outbyte(command);
    xil_printf("\r\n");

    switch (command)
    {
    case 'h':
    case 'H':
        print_help();
        break;

    case 'i':
    case 'I':
        xil_printf("Current count = %u\r\n", *count);
        break;

    case 'r':
    case 'R':
        *count = 0U;
        xil_printf("Count reset to 0\r\n");
        break;

    case 'q':
    case 'Q':
        *periodic_print_enabled = 0;
        xil_printf("Periodic printing paused\r\n");
        break;

    case 'c':
    case 'C':
        *periodic_print_enabled = 1;
        xil_printf("Periodic printing continued\r\n");
        break;

    default:
        xil_printf("Unknown command '%c'. Press h for help.\r\n", command);
        break;
    }
}

int main()
{
    unsigned int count = 0U;
    unsigned int elapsed_ms = 0U;
    int periodic_print_enabled = 1;

    init_platform();

    xil_printf("Zynq UART command experiment\r\n");
    print_help();
    xil_printf("Hello Zynq, count = %u\r\n", count++);

    while (1)
    {
        /*
         * inbyte() is blocking, so call it only after the UART status
         * register says that at least one byte is ready to be read.
         */
        while (XUartPs_IsReceiveData(STDIN_BASEADDRESS))
        {
            char command = inbyte();

            /* Most serial terminals append CR/LF when Enter is pressed. */
            if ((command != '\r') && (command != '\n'))
            {
                handle_command(command, &count, &periodic_print_enabled);
            }
        }

        usleep(POLL_INTERVAL_US);

        if (periodic_print_enabled != 0)
        {
            elapsed_ms += POLL_INTERVAL_US / 1000U;
            if (elapsed_ms >= PRINT_INTERVAL_MS)
            {
                elapsed_ms -= PRINT_INTERVAL_MS;
                xil_printf("Hello Zynq, count = %u\r\n", count++);
            }
        }
        else
        {
            /* Continue starts a fresh one-second period. */
            elapsed_ms = 0U;
        }
    }

    cleanup_platform();
    return 0;
}

```

#### 6.3.6 代码执行路径

接收一个 `r` 命令时，数据按以下路径移动：

```text
电脑发送 ASCII 'r'
→ MIO48/MIO49 对应的 PS UART1
→ UART RX FIFO
→ XUartPs_IsReceiveData() 发现 RXEMPTY=0
→ inbyte() 读取 FIFO
→ command 保存字符 'r'
→ handle_command() 进入 case 'r'
→ 通过 *count 将主函数中的计数值清零
→ xil_printf() 经 outbyte() 发送确认信息
```

周期打印采用三个变量配合：

```text
count                    下一次要打印的计数值
elapsed_ms               已累计的轮询时间
periodic_print_enabled   1=运行，0=暂停
```

`q` 只把状态改为 0，主循环和 UART 检查仍然运行，所以随后还能接收 `c`。暂停期间 `elapsed_ms` 被清零，恢复后重新等待完整的一秒。

当前软件计时是近似计时：命令处理和串口打印本身也会消耗时间，但没有计入 `elapsed_ms`。实验阶段可以接受；需要精确周期时应使用 SCU Timer、TTC 或硬件计时器中断。

#### 6.3.7 上板练习与验收

串口参数设置为 `115200, 8N1, 无流控`，依次完成：

1. 启动后确认先打印帮助和 `count = 0`。
2. 输入 `i`，解释为什么显示的是“下一次要打印的值”。
3. 输入 `r`，观察下一次周期输出是否从 0 开始。
4. 输入 `q`，等待至少 3 秒，确认周期输出停止。
5. 暂停期间输入 `i`，确认命令接收仍然工作。
6. 输入 `c`，确认大约一秒后恢复输出。
7. 输入大写 `H` 和一个未定义字符，检查大小写兼容和默认分支。
8. 分别使用“无换行”和“附加 CRLF”发送，解释过滤 `\r`、`\n` 的原因。

调试时建议观察：

- 在 `handle_command()` 设置断点，查看 `command`、`count` 和 `periodic_print_enabled`。
- 单步进入 `inbyte()`，追踪到 `XUartPs_RecvByte()` 和 `Xil_In32()`。
- 观察 UART 状态寄存器 `0xE000102C`；不要持续监视 FIFO 地址 `0xE0001030`，因为读取 FIFO 可能消费字符。
- 只修改一个命令的提示字符串，重新生成和下载 ELF，验证无需重新生成 bitstream。

完成后应能不看代码回答：

- 字符 `'h'` 和字符串 `"h"` 有什么区别？
- 字符串h后默认加字符串结束位\0
- 为什么 `inbyte()` 不能在主循环中无条件调用？
- 如果没有数据会导致一直卡在while循环
- `&count` 与 `*count` 分别是什么意思？
- 取地址和通过地址访问变量count
- `RXEMPTY` 与 STM32 常见的 `RXNE` 有什么区别？
- 都是寄存器中的某一位 有数据时变量的值不同
- 为什么执行 `q` 后仍能接收 `c`？
- `q` 改变的是打印状态，不是主循环运行状态
- 为什么该软件计时不是精确的一秒？
- 有函数的延迟



### 6.4 实验三：SDK 调试

完成以下练习：

1. 在 `main()` 设置断点。
2. 单步执行 `init_platform()` 前后的代码。
3. 在变量窗口观察 `count`。
4. 暂停处理器并查看调用栈。
5. 修改变量值后继续运行。

完成标准：不仅会点击 Run，还知道程序当前停在哪里、为什么停在那里。

### 6.5 STM32 迁移验收

完成 P01 后，应能独立回答：

- `main()` 之前是谁完成 DDR、时钟和 MIO 初始化的？
- SDK 中的 BSP 与 STM32 HAL/CMSIS 包分别有哪些相似和不同？
- `xil_printf()`、`inbyte()`、`outbyte()` 最终访问的是哪个 UART 基地址？
- 哪些修改只需重新生成 ELF，哪些修改必须重新导出 HDF 或生成 bitstream？

如果这些问题都能回答，并且能够不照抄地重新写出简化版串口命令循环，就可以进入 P02；否则回到 6.3.2～6.3.7 按数据路径逐层调试。

## 7. 阶段二：P02 PS GPIO

工程入口：[`P02_GPIO/GPIO.xpr`](./P02_GPIO/GPIO.xpr)

### 7.1 目标

- 理解 MIO 与 PS GPIO。
- 学会配置输入、输出和输出使能。
- 能够读取按键并控制 LED。
- 学会消抖和非阻塞状态机。

### 7.2 原样运行示例

先不修改代码，完成一次完整下载并观察：

- 按 S1 时串口输出什么。
- 按 S2 时串口输出什么。
- MIO37、MIO38 和 MIO15 对应哪些 LED。
- LED 是高电平亮还是低电平亮。

把实际现象写进学习笔记，不要只相信变量名。

### 7.3 GPIO 驱动 API

重点理解：

```c
XGpioPs_LookupConfig();
XGpioPs_CfgInitialize();
XGpioPs_SetDirectionPin();
XGpioPs_SetOutputEnablePin();
XGpioPs_WritePin();
XGpioPs_ReadPin();
```

与 STM32 GPIO 配置的对应关系：

| STM32 操作 | `XGpioPs` 操作 |
|---|---|
| 选择输入/输出模式 | `XGpioPs_SetDirectionPin()` |
| 使能输出驱动 | `XGpioPs_SetOutputEnablePin()` |
| 写 ODR/BSRR | `XGpioPs_WritePin()` |
| 读 IDR | `XGpioPs_ReadPin()` |
| 查找外设实例和基地址 | `XGpioPs_LookupConfig()` |
| 初始化驱动句柄 | `XGpioPs_CfgInitialize()` |

建议额外做一次寄存器追踪练习：从 `XGpioPs_WritePin()` 的声明进入驱动源码，找到最终访问的寄存器偏移，并与 Zynq-7000 TRM 中的 GPIO 寄存器表核对。目标不是绕开驱动，而是建立“BSP API → 基地址 → 寄存器”的完整映射。

STM32 GPIO 同步复习：

- 模式决定管脚是输入、输出、复用还是模拟；STM32F4 等系列常见 `MODER`，F1 常见 `CRL/CRH`。
- 输出类型分推挽和开漏；开漏输出通常需要上拉。
- `PUPDR` 或对应配置决定上拉、下拉或浮空。
- `IDR` 读取外部实际输入，`ODR` 表示输出锁存值，`BSRR` 可原子置位/清零。
- 有效电平由外部电路决定，LED 可能低电平点亮，变量名不能代替原理图验证。
- 按键机械触点会抖动，软件消抖应确认状态持续稳定一段时间，而不是简单延时后默认有效。

### 7.4 改造任务

- [ ] 红绿 LED 每 500 ms 交替闪烁。
- [ ] 按 S1 加快闪烁。
- [ ] 按 S2 减慢闪烁。
- [ ] 串口输入 `r`、`g`、`0` 控制红灯、绿灯和关闭。
- [ ] 对按键增加 20～50 ms 软件消抖。
- [ ] 去掉主循环中的长时间阻塞延时。

### 7.5 非阻塞程序思想

初学代码经常这样写：

```c
LED_ON();
sleep(1);
LED_OFF();
sleep(1);
```

这会导致处理器在延时期间不能及时处理串口和按键。改进方向是：

```text
读取当前时间
→ 判断是否到达下一次动作时间
→ 到达时改变状态
→ 继续处理串口和按键
```

## 8. 阶段三：定时器和中断

### 8.1 目标

- 理解轮询与中断的区别。
- 使用 SCU Timer 产生周期中断。
- 使用 GIC 管理中断。
- 编写短小、可控的中断服务函数。

STM32 迁移提示：把 GIC 暂时类比为功能更完整的 NVIC，把 SCU Timer 类比为通用定时器。已有的 ISR 设计原则仍然适用，但 Zynq 需要显式建立 `Xil_Exception`、GIC 和外设中断源三层连接，不能只配置一个 NVIC 通道。

STM32 中断同步复习：

```text
外设事件发生
→ 外设置位状态/挂起标志
→ 外设中断使能允许请求
→ NVIC/GIC 允许并仲裁该中断
→ CPU 保存现场并进入 ISR
→ ISR 清除中断源并记录事件
→ 返回主程序处理耗时业务
```

必须区分“外设标志位”和“中断控制器挂起状态”。如果只退出 ISR 而没有清除外设中断源，电平型中断会立即再次进入。ISR 与主循环共享的简单标志通常声明为 `volatile`，但 `volatile` 只阻止编译器省略访问，并不自动保证多步操作的原子性。

### 8.2 实验任务

- [ ] SCU Timer 每 1 ms 产生一次中断。
- [ ] 由 1 ms 节拍合成 500 ms LED 翻转。
- [ ] 主循环继续处理串口命令。
- [ ] 为按键增加 GPIO 中断。
- [ ] 统计一秒内的中断次数，并通过串口显示。

### 8.3 中断程序规则

- 中断函数中尽量只清中断、保存数据或设置标志。
- 不在中断函数中执行长时间 `sleep()`。
- 不在高频中断中大量打印。
- 共享变量根据场景使用 `volatile`。
- 查清中断触发方式以及中断清除方法。

验收条件：LED 能周期闪烁，同时串口输入仍能及时响应。

## 9. 阶段四：P03 SD 卡与 FATFS

工程入口：[`P03_SD_TEST/SD_TEST.xpr`](./P03_SD_TEST/SD_TEST.xpr)

主要源码：

- [`P03_SD_TEST/SD_TEST.sdk/SD_TEST/src/main.c`](./P03_SD_TEST/SD_TEST.sdk/SD_TEST/src/main.c)
- [`P03_SD_TEST/SD_TEST.sdk/SD_TEST/src/sd/sd.c`](./P03_SD_TEST/SD_TEST.sdk/SD_TEST/src/sd/sd.c)

### 9.1 目标

- 挂载 FAT 文件系统。
- 创建、写入、读取和关闭文件。
- 正确检查返回值和实际读写字节数。
- 处理无卡、坏卡和文件不存在等错误。

如果已经在 STM32 上使用过 FatFs，可以跳过文件 API 的基础学习，直接关注 `XSdPs` 底层驱动、SD 控制器基地址、实际读写字节数以及 Cache 对 DMA/缓冲区的影响。`f_mount()`、`f_open()`、`f_read()`、`f_write()` 的使用方式基本可以直接迁移。

如果 FatFs 也需要复习，应先分清三层：底层 SD 控制器负责按块收发，`diskio` 将其适配为逻辑扇区读写，FatFs 再把扇区组织成文件和目录。每次读写都要检查 `FRESULT` 和实际字节数 `br/bw`；缓冲区容量必须至少覆盖请求长度，并为 C 字符串额外预留一个 `\0`。

### 9.2 基础实验

1. 准备 FAT32 SD 卡。
2. 下载 P03 程序。
3. 检查是否输出 `success to open SD Card`。
4. 检查 SD 卡根目录是否生成 `test.txt`。
5. 重新读取文件并通过串口显示。

### 9.3 修正现有示例的越界风险

现有代码使用：

```c
SD_Transfer_write(FILE, (u32)src_str, (len / 256 + 1) * 256);
```

如果字符串只有几十字节，这段代码仍会从该地址读取 256 字节，可能读到字符串数组之外。建议改为：

```c
SD_Transfer_write(FILE, (u32)src_str, len);
```

如果确实需要保存结尾的 `\0`：

```c
SD_Transfer_write(FILE, (u32)src_str, len + 1);
```

接收缓冲区要预留结尾并初始化：

```c
char dest_str[64] = {0};

SD_Transfer_read(FILE, (u32)dest_str, len);
dest_str[len] = '\0';
```

还应检查 `f_read()` 的 `br` 和 `f_write()` 的 `bw` 是否等于期望长度。

### 9.4 进阶任务

- [ ] 追加写入而不是每次覆盖文件。
- [ ] 每秒记录一次计数值。
- [ ] 将数据保存成 CSV。
- [ ] 每写若干条数据后同步文件。
- [ ] 检测写满或写失败。
- [ ] 设计安全停止记录的命令，避免正在写入时拔卡。

阶段作品：一个通过串口控制启停的 SD 卡数据记录器。

## 10. 阶段五：P04 从 SD 卡独立启动

工程入口：[`P04_SD_BOOT/SD_BOOT.xpr`](./P04_SD_BOOT/SD_BOOT.xpr)

启动描述：[`P04_SD_BOOT/SD_BOOT.sdk/SD_BOOT/bootimage/SD_BOOT.bif`](./P04_SD_BOOT/SD_BOOT.sdk/SD_BOOT/bootimage/SD_BOOT.bif)

### 10.1 启动过程

```text
上电
→ Zynq BootROM 读取启动模式管脚
→ 从 SD 卡读取 BOOT.BIN
→ 运行 FSBL
→ 初始化时钟、MIO 和 DDR
→ 可选：配置 PL bitstream
→ 运行应用程序 ELF
```

STM32 通常由片内 BootROM 直接进入 Flash 中的启动代码；Zynq 的启动链更长。可以把 FSBL 类比为“增强版 `SystemInit()` + 镜像加载器”：它不仅设置时钟和 MIO，还初始化 DDR、可选地配置 PL，并把后续程序搬运到目标地址。理解这一差异是从 MCU 迁移到 SoC 的关键。

STM32 启动知识同步复习：CPU 复位后从向量表取得初始栈指针和 Reset Handler 地址；启动代码复制 `.data`、清零 `.bss`、配置系统环境后调用 `main()`。链接脚本决定各段放在 Flash、SRAM 或 DDR 的位置。Zynq 保留这些软件启动概念，但在它们之前增加了 BootROM、FSBL、DDR 初始化和可选的 PL 配置。

### 10.2 注意旧路径

仓库中的 `.bif` 保存了原作者电脑上的 `E:\S9\...` 绝对路径，不能直接用于当前电脑。应在 SDK 中执行：

```text
Xilinx Tools
→ Create Boot Image
```

重新选择当前电脑上的文件。

PS-only 程序的排列：

```text
[bootloader] FSBL.elf
application.elf
```

包含 PL 逻辑的程序通常排列为：

```text
[bootloader] FSBL.elf
system.bit
application.elf
```

### 10.3 实验步骤

- [ ] 创建 Zynq FSBL 工程。
- [ ] 编译 FSBL 和应用程序。
- [ ] 重新创建启动镜像配置。
- [ ] 生成 `BOOT.BIN`。
- [ ] 将 `BOOT.BIN` 复制到 FAT32 SD 卡根目录。
- [ ] 关闭板子电源。
- [ ] 将启动跳线调整为 SD 启动模式。
- [ ] 插入 SD 卡并打开串口终端。
- [ ] 重新上电并观察输出和 LED。
- [ ] 拔掉 JTAG 后再次验证独立启动。

验收条件：不连接 JTAG，上电后程序也能自动运行。

## 11. 阶段六：Verilog、仿真和 PL

P01～P04 主要是 PS 实验。完成它们之后，才正式进入 FPGA 逻辑部分。

这是 STM32 开发者最需要投入时间的阶段，不能因为熟悉寄存器配置而跳过。PS 代码仍是按指令顺序执行的软件，PL 中的各个逻辑模块却同时工作；这里应刻意停止使用“每行代码依次执行”的思维。

### 11.1 学习内容

- 组合逻辑和时序逻辑。
- `wire`、`reg` 和端口。
- 阻塞赋值 `=` 与非阻塞赋值 `<=`。
- 时钟和复位。
- 计数器、PWM、移位寄存器和状态机。
- Testbench 和行为仿真。
- XDC 管脚约束和时序约束。

### 11.2 推荐实验顺序

1. 与门和多路选择器。
2. 同步计数器。
3. 周期使能脉冲。
4. PWM 输出。
5. 按键消抖。
6. 有限状态机。
7. 为每个模块编写 Testbench。

### 11.3 PL 输出实验

选择一个经过原理图确认、确实连接到 PL 的安全管脚：

```text
PS FCLK_CLK0 或可靠板载时钟
→ PL 计数器
→ 周期使能
→ PL 输出管脚
→ 外接 LED 和限流电阻，或示波器
```

XDC 形式示例：

```tcl
set_property PACKAGE_PIN <实际封装管脚> [get_ports led]
set_property IOSTANDARD <与该Bank电压一致的标准> [get_ports led]
```

不能照抄其他开发板的管脚和 `LVCMOS33`。必须先核对 S9 原理图、封装管脚和 Bank 电压。

## 12. 阶段七：AXI GPIO

### 12.1 硬件结构

```text
Zynq PS
 ├─ FCLK_CLK0 ──────────────┐
 └─ M_AXI_GP0 → AXI互连 → AXI GPIO → PL输出
                             └──────── 时钟和复位
```

### 12.2 实验目标

- 在 PS 配置中启用 `M_AXI_GP0` 和 `FCLK_CLK0`。
- 添加 AXI GPIO IP。
- 自动连接 AXI、时钟和复位。
- 在 Address Editor 中分配地址。
- 生成 bitstream 并导出硬件。
- 在 SDK 中使用 `XGpio` 驱动。

### 12.3 必须理解的概念

- AXI GPIO 是 PL 外设，不是 `XGpioPs`。
- `XGpioPs` 操作 PS 的 MIO/EMIO GPIO。
- `XGpio` 操作 PL 中的 AXI GPIO IP。
- ARM 通过读写地址访问 PL 中的寄存器。
- 地址信息最终进入 SDK 的 `xparameters.h`。

STM32 迁移类比：AXI GPIO 可以看作“由你放进芯片、再分配基地址的 GPIO 外设”。Vivado 中添加 IP、连接时钟复位和分配地址，相当于芯片厂商在设计 STM32 时完成的外设挂载工作；区别在于 Zynq 允许开发者自己完成这一步。

验收条件：串口输入命令后，PS 通过 AXI 改变 PL 输出。

## 13. 阶段八：自定义 AXI4-Lite IP

创建一个含四个寄存器的自定义 IP：

| 地址偏移 | 建议用途 |
|---:|---|
| `0x00` | 控制寄存器 |
| `0x04` | 状态寄存器 |
| `0x08` | PWM 周期 |
| `0x0C` | PWM 占空比 |

软件端先使用直接地址访问：

```c
Xil_Out32(BASE_ADDR + 0x08, 1000);
Xil_Out32(BASE_ADDR + 0x0C, 500);

status = Xil_In32(BASE_ADDR + 0x04);
```

本阶段可以充分利用已有的寄存器经验。把自定义 IP 当作自己设计的一颗 STM32 外设：先写寄存器表，再定义偏移、位域、访问属性、复位值和副作用，最后才写 RTL 与 C 驱动。建议寄存器表至少包含 `Name`、`Offset`、`Bit`、`R/W`、`Reset` 和 `Description` 六列。

寄存器基础同步复习：`BASE + OFFSET` 定位一个寄存器，掩码和移位用于提取位域；修改部分位时通常使用读—改—写，但对 W1C（写 1 清除）、只写和具有副作用的寄存器不能盲目读—改—写。MMIO 访问必须使用 Xilinx 提供的 `Xil_In32/Xil_Out32` 或正确的 `volatile` 封装，不能让编译器把硬件访问优化掉。

完成以下任务：

- [ ] 软件写控制寄存器，PL 改变输出。
- [ ] 软件读取 PL 状态和计数值。
- [ ] 为寄存器定义位域和复位值。
- [ ] 防止保留位被意外改写。
- [ ] 编写简单的 C 驱动封装。
- [ ] 通过串口修改 PWM 参数。

## 14. 阶段九：PL 中断和 ILA

### 14.1 PL 中断 PS

让 PL 中的事件经 `IRQ_F2P` 进入 PS：

- [ ] PL 计数器产生事件。
- [ ] 中断信号连接到 `IRQ_F2P`。
- [ ] PS 端 GIC 注册中断服务函数。
- [ ] 中断服务函数确认并清除中断。
- [ ] 主循环显示累计事件数。

要特别注意：

- 电平中断和脉冲中断的区别。
- 中断信号跨时钟域处理。
- 中断清除时机。
- 重复进入中断的原因。

### 14.2 ILA 调试

将 ILA 接入：

- AXI 写地址和写数据通道。
- 自定义寄存器值。
- 中断请求和清除信号。
- 状态机状态。
- 外设输入输出。

练习设置触发条件，捕获“软件写了寄存器但硬件没有反应”的全过程。

## 15. 阶段十：AXI DMA 和高速数据

等 AXI4-Lite 和 ILA 熟练后，再进入 DMA：

```text
PL 数据发生器
→ AXI4-Stream
→ AXI DMA
→ PS DDR
→ Cortex-A9 处理或写入 SD 卡
```

学习重点：

- AXI4-Lite：低速控制寄存器。
- AXI4 Memory Mapped：存储器地址访问。
- AXI4-Stream：连续数据流。
- AXI DMA：在 PL 与 DDR 之间搬运数据。
- `TVALID`、`TREADY`、`TLAST`。
- HP Port。
- Cache flush 和 invalidate。
- DMA 完成中断。

与 STM32 DMA 相比，Zynq DMA 最容易漏掉的是 Cache 一致性：CPU 写完发送缓冲区后通常要 flush，DMA 写完接收缓冲区后 CPU 读取前通常要 invalidate。即使寄存器配置和中断全部正确，漏掉这一步仍可能看到旧数据。

STM32 DMA 同步复习：先确认数据方向、源地址、目的地址、传输宽度、数量、地址是否递增以及完成/错误标志；循环模式和双缓冲用于连续数据。进入 Zynq 后这些概念仍然成立，但 AXI DMA 还涉及 AXI-Stream 握手、DDR 地址、HP Port、突发传输和 Cache 一致性。

推荐任务：PL 连续生成递增数据，经 DMA 写入 DDR，ARM 检查每个数据是否正确。

## 16. 阶段十一：U-Boot 和 Linux

### 16.1 启动链

```text
BootROM
→ FSBL
→ bitstream
→ U-Boot
→ Linux Kernel
→ Device Tree
→ Root Filesystem
→ 用户程序
```

### 16.2 推荐顺序

1. 掌握 U-Boot 串口命令。
2. 从 SD 卡加载内核和设备树。
3. 阅读 Linux 启动日志。
4. 理解设备树中的 UART、SD、以太网和 GPIO。
5. 使用 UIO 或受控的寄存器映射访问 AXI 外设。
6. 编写简单平台驱动。
7. 添加中断支持。
8. 最后再研究 DMA 驱动。

Vivado 2017.4 最好配合同代 Xilinx 工具链和 BSP。初学阶段不要频繁跨版本转换工程。

## 17. STM32 基础巩固与 Zynq 迁移八周计划

这份计划不再假设 STM32 基础已经非常扎实。每周同时包含“基础复习”和“Zynq 新知识”，以能解释、能修改、能调试为完成标准，不以背诵寄存器为目标。

| 周次 | 基础复习 | Zynq 实验 | 验收作品 |
|---|---|---|---|
| 第 1 周 | C 类型、ASCII、指针、函数、`switch`、UART 状态位 | P01 循环打印和 6.3 串口命令 | 能逐层解释 `command` 从 UART FIFO 到命令函数的路径 |
| 第 2 周 | 位运算、GPIO 模式、有效电平、按键消抖、非阻塞状态机 | P02 PS GPIO | 串口和按键共同控制 LED，主循环无长延时 |
| 第 3 周 | 定时器时基、ISR、标志位、NVIC、`volatile` | SCU Timer、GIC、GPIO 中断 | 中断产生节拍，主循环处理命令和业务 |
| 第 4 周 | 缓冲区、数组边界、返回值、FatFs 分层 | P03 SD/FatFs、P04 FSBL 与 BOOT.BIN | 可脱离 JTAG 启动的 SD 数据记录器 |
| 第 5 周 | 组合逻辑、触发器、时钟、复位、状态机 | Verilog、Testbench、XDC | 有自检 Testbench 的 PL PWM 或计数器 |
| 第 6 周 | 基地址、偏移、位域、MMIO、寄存器读写属性 | AXI GPIO、自定义 AXI4-Lite | PS 通过自定义寄存器控制 PL 外设 |
| 第 7 周 | 边沿/电平中断、同步、竞争和调试观测 | IRQ_F2P、跨时钟域、ILA | 能用 ILA 证明中断和寄存器访问过程 |
| 第 8 周 | DMA 方向、宽度、长度、缓冲区和完成中断 | AXI-Stream、AXI DMA、HP Port、Cache | PL 数据经 DMA 写入 DDR 并由 CPU 校验 |

每周建议遵循“阅读 20%—动手 60%—复盘 20%”。如果某周的复习题无法用自己的话回答，就延长该阶段，不急于进入下一周。八周完成后再学习 U-Boot、Linux、设备树和平台驱动。

## 18. 每个实验的标准闭环

每个实验都执行下面的过程：

1. 写清楚实验目标。
2. 画出数据流或信号流。
3. 在原理图中找到物理连接。
4. 确认管脚、电压、方向和有效电平。
5. 从上一份已验证工程复制出新版本。
6. 编译并认真查看警告。
7. 使用 JTAG 下载验证。
8. 使用串口、万用表、示波器或 ILA 观察结果。
9. 故意制造一个小错误并练习定位。
10. 记录现象、原因和解决方法。
11. 写出验收结论。

实验记录模板：

```markdown
## 实验名称

### 目标

### 硬件连接

### 管脚和电压

### 软件与工程版本

### 设计框图

### 操作步骤

### 预期现象

### 实际现象

### 遇到的问题

### 原因与解决办法

### 最终结论
```

## 19. 推荐的个人工程目录

不要一直覆盖原始示例。建议建立自己的学习目录：

```text
S9_STUDY/
├─ P01_UART_PRINT/
├─ P02_UART_COMMAND/
├─ P03_PS_GPIO/
├─ P04_TIMER_IRQ/
├─ P05_SD_FATFS/
├─ P06_SD_BOOT/
├─ P07_PL_COUNTER/
├─ P08_AXI_GPIO/
├─ P09_CUSTOM_AXI/
├─ P10_PL_INTERRUPT/
├─ P11_ILA/
└─ P12_DMA/
```

每个目录保留：

- Vivado 工程。
- SDK Workspace。
- 对应源码。
- 引脚表。
- 实验说明 Markdown。
- 串口输出或 ILA 截图。
- 已知问题和解决方法。

## 20. 安全注意事项

- USB-TTL 只连接 TX、RX 和 GND，通常不要连接 VCC。
- 确认 USB-TTL 使用与板子兼容的 3.3 V 逻辑电平。
- 调整启动跳线前先关闭板子电源。
- 不要在没有备份和恢复方案时写入板载 NAND。
- 初学阶段优先使用 JTAG 和 SD 卡。
- 不要照抄其他 Zynq 开发板的 XDC 管脚。
- PL 管脚输出前必须确认 Bank 电压和外部电路。
- 不要将两个推挽输出直接连接在一起。
- 不确定的板载接口先使用万用表和原理图确认。
- 保留一份能够正常运行的 P01 工程作为调试基准。
- 新 bitstream 可能改变 PL 管脚状态，下载前检查约束和默认输出。

## 21. 当前应执行的下一步

当前先完成实验 6.3 的“理解验收”，不要只满足于代码已经能够编译：

1. 阅读 6.3.2，亲手写一个 `char`、指针和 `switch` 的最小例子。
2. 在 SDK 中进入 `inbyte()`，继续追踪 `XUartPs_RecvByte()`、`XUartPs_IsReceiveData()` 和 `Xil_In32()`。
3. 画出 `电脑 → UART RX FIFO → command → handle_command()` 的数据路径。
4. 依次测试 `h/i/r/q/c`、大写命令、未知字符和 CRLF。
5. 在 `handle_command()` 设置断点，观察 `&count` 与 `*count`。
6. 在工程副本中暂时将 UART 检查改成无条件 `inbyte()`，观察周期打印为何停止，然后恢复正确代码。
7. 不看参考代码，重新写出只支持 `i/r/q/c` 的简化版本。
8. 回答 6.3.7 末尾的六个问题，并将答案记录到实验笔记。

完成 6.3 的理解验收后，再进入 `P02_GPIO`，把相同的学习方法用于 `XGpioPs API → 驱动源码 → 寄存器 → MIO 管脚`。

## 22. 进度检查表

- [x] JTAG 能识别 `arm_dap_0` 和 `xc7z010_1`
- [x] 能够 Program Device
- [x] 能够初始化 PS 并下载 ELF
- [x] UART 能看到 Hello World
- [x] Hello World 循环计数
- [ ] UART 接收和命令解析（代码已完成，正在进行理解验收）
- [ ] P02 GPIO 原样运行
- [ ] GPIO 自主改造
- [ ] 定时器中断
- [ ] GPIO 中断
- [ ] P03 SD 读写
- [ ] SD 数据记录器
- [ ] P04 SD 独立启动
- [ ] Verilog Testbench
- [ ] PL 输出实验
- [ ] AXI GPIO
- [ ] 自定义 AXI4-Lite IP
- [ ] PL 中断 PS
- [ ] ILA 调试
- [ ] AXI DMA
- [ ] U-Boot 和 Linux

---

建议始终遵循一个标准：每完成一级，都应该能在不照抄教程的情况下重新建立一个最小工程，并能够解释数据从哪里来、经过什么模块、最终到哪里去。

## 23. 学习控制层

本文件继续作为教程内容草案。为避免学习进度仍然被例程顺序主导，新增 [`learning/INDEX.md`](./learning/INDEX.md) 作为学习系统入口，其中包含项目地图、课程先修关系、掌握证据、误区记录、复习队列、基线诊断和迁移挑战。

后续不再仅通过本文件的勾选项判断进度；`learning/MASTERY.md` 中的无提示复现、调试和迁移证据才是进入下一阶段的主要依据。
