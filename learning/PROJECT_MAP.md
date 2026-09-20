# 当前项目地图

## 已确认的项目边界

| 项目 | 当前证据 |
|---|---|
| 控制板 | 蚂蚁 S9 控制板，仓库含 `AntMiner_ControlBoard_XC7010_V1.01控制板.pdf` |
| SoC | Vivado 工程目标器件均为 `xc7z010clg400-1` |
| 处理系统 | Zynq-7000 Processing System 7，Cortex-A9 裸机 BSP |
| 工具链 | 现有教程和工程按 Vivado 2017.4、Xilinx SDK 2017.4 组织 |
| 当前已记录现象 | JTAG识别、Program Device、PS初始化、ELF下载、UART输出已完成 |

“工程里存在代码”不等于“硬件已经验证”，也不等于“学习者已经掌握”。后两项必须分别记录实验现象和独立能力证据。

## 四个现有工程

| 工程 | 主要入口 | 可承载的核心知识 | 当前状态 |
|---|---|---|---|
| P01_HELLO | `HELLO.sdk/HELLO/src/helloworld.c` | UART轮询、字符与ASCII、指针、状态机、BSP到寄存器调用链 | 程序已改造；理解验收未完成 |
| P02_GPIO | `GPIO.sdk/GPIO/src/main.c` | PS GPIO、MIO、有效电平、消抖、非阻塞时间状态 | 代码存在；尚未记录独立上板验收 |
| P03_SD_TEST | `SD_TEST.sdk/SD_TEST/src/main.c`、`src/sd/sd.c` | SD控制器、块设备、FatFs、缓冲区长度、返回值与资源释放 | 原始例程存在；需先修正安全问题再验收 |
| P04_SD_BOOT | `SD_BOOT.sdk/SD_BOOT/src/main.c`、`bootimage/SD_BOOT.bif` | BootROM、FSBL、PS初始化、镜像布局、脱离JTAG启动 | 工程存在；启动链证据尚未记录 |

## 项目文件的证据权重

1. **实测证据**：串口日志、JTAG状态、ILA波形、示波器/万用表读数、SD卡文件结果。
2. **用户源码**：各 SDK 应用的 `src/`、手写 RTL、XDC、BIF 和实验记录。
3. **设计配置**：`.bd`、PS7配置、Address Editor、导出的 HDF/HWH。
4. **生成代码**：BSP驱动、`xparameters.h`、`ps7_init.*`；适合追踪实现，不代表用户设计意图。
5. **构建产物与日志**：`.bit`、`.elf`、`.o`、runs、cache、日志；只用于复现和诊断。

## 已发现的学习素材与风险

### P01 UART

- 主循环先用 `XUartPs_IsReceiveData()` 检查FIFO，再调用阻塞式 `inbyte()`。
- `count` 和 `periodic_print_enabled` 通过指针传入命令处理函数，适合验证“地址、解引用和状态修改”。
- 10 ms轮询和串口打印耗时没有形成严格时基，适合引出轮询、漂移、定时器和中断。
- 部分中文源码注释在 UTF-8 读取时乱码，需在继续编辑前确认原始编码，避免二次破坏。

### P02 GPIO

- 使用 `XGpioPs` 操作编号15、37、38、47、51，适合追踪 API、驱动、寄存器和 MIO 电气路径。
- 按键消抖仍包含短暂阻塞延时；LED保持时间改用 `XTime`，适合比较阻塞延时与时间状态机。
- P04源码把37/38命名为 `EMIO_LED_*`，但仅凭命名不能证明它们属于EMIO；必须用PS配置、原理图和实际GPIO编号核验。

### P03 SD/FatFs

- `SD_Transfer_write(FILE, (u32)src_str, (len/256+1)*256)` 会要求 FatFs 从一个较短字符串后继续读取，存在越界读取风险。
- 读写函数没有验证实际读取/写入字节数 `br`/`bw`，错误分支也可能在关闭文件前返回。
- 这是学习“指针不携带容量、文件API成功不代表长度正确、错误路径也要释放资源”的真实案例。

### P04 SD启动

- BIF 当前包含 FSBL 和应用 ELF，使用 `E:\S9\...` 绝对路径，换机器或目录后不可复用。
- 当前BIF未列出 bitstream；是否合理取决于该启动实验是否需要配置PL，不能机械补入。
- `FSBL -> 应用`只是启动链的一部分；还需要通过启动模式、串口日志和SD卡内容形成实测证据。

## 尚未被现有例程覆盖的主要能力

- GIC、SCU Timer/TTC、异常入口和中断生命周期；
- Verilog/SystemVerilog、Testbench、XDC、时序分析、复位和CDC；
- AXI4-Lite寄存器设计、AXI-Stream和背压；
- PS–PL中断、ILA和跨层调试；
- AXI DMA、HP端口、DDR和Cache一致性；
- BootROM/FSBL/U-Boot/Linux、设备树和平台驱动；
- 从需求、带宽和时延约束反推PS/PL划分与系统架构。

这些缺口已经拆成 `CURRICULUM.md` 的能力节点和 `PROJECT_BACKLOG.md` 的 P05–P40，不再等待“找到现成例程”后才学习。板级可用资源、证据冲突和实验风险见 `research/BOARD_CAPABILITY_MATRIX.md`。
