# 2026-10-16 P03 安全副本、SDK 配置与 SD 初始化故障记录

## 本次收尾

- 用户要求：结束本次课程，记录并同步。到此停止新的硬件操作，不启动提醒或后台排查。
- 课程节点：T00/T01、T04、C01、B00/B01；项目锚点为 P03 SD/FatFs 安全审计。
- 辅助程度：AI 提供安全函数、驱动诊断插桩、工程检查和操作步骤；学习者在 Vivado/SDK 中修改、构建、上板、更换卡并用万用表测量。
- 结论：安全代码构建完成，原版和安全副本均未完成文件读写；平台 `_2` 下 CMD8/CMD55 等待响应超时，根因尚未确定。
- 掌握等级：T00/T01、C01、B00/B01 保持 L1；T04 保持 L0（有辅助实验记录，尚无独立设计/解释证据）。用户在本次结束时未做闭卷验收。
- 日期说明：本记录按 2026-10-16（Asia/Shanghai）收尾；此前微型复测保留在 2026-09-24 的独立记录中，不改写历史日期。

## 环境与本地恢复路径

- 仓库根目录：`D:/Documents/ChatGPT/zynq/S9miner_sample`。
- 工具：Vivado 2017.4、Xilinx SDK 2017.4；本次追踪 `xilffs_v3_7`（FatFs R0.10b）和 `sdps_v3_3`。
- 首个安全软件副本：`P03_SD_TEST/SD_TEST.sdk/SD_Test_Safe`，BSP 为 `SD_Test_Safe_bsp`。
- 当前硬件副本：`SD_TEST_SAFE_HW/SD_TEST_SAFE_HW.xpr`。
- 当前 SDK 工作区：`SD_TEST_SAFE_HW/SD_TEST_SAFE_HW.sdk`。
- 当前安全应用与 ELF：`SD_Test_Safe/src`、`SD_Test_Safe/Debug/SD_Test_Safe.elf`。
- 新硬件平台：`zynq_wrapper_hw_platform_2`，使用其 `ps7_init.tcl` 和 `system.hdf`。
- 注意：现有 BSP 的 `.sdkproject` 仍关联旧平台 `_0`；不能声称已把全部 BSP 重定向到 `_2`。本次以运行配置选择 `_2` 的初始化脚本，并明确指定已有 ELF 完成对照。
- 切换平台会过滤应用项目列表，甚至清空 Application。列表为空时可在 Application 的 Browse 中直接选择 ELF 的完整路径。继续运行必须核对 Download、ELF 和日志。

本次同步保存学习文档、必要的历史证据及以下实验步骤；完整 Vivado/SDK 副本、缓存、BSP 生成代码和构建产物留在本地，不整包提交。下列本地路径不代表实验工程已全部上传。

## 安全软件实现（辅助完成）

安全 `sd.h/sd.c` 采用文件名、缓冲区地址、容量、请求量和 `UINT *ActualLength` 接口；源缓冲区为 `const void *`，目标缓冲区为 `void *`。实现包含：

- 实际长度先清零，参数和容量检查；
- 检查 `f_open/read/write` 返回值以及实际字节数量；
- 打开成功后统一进入清理出口，只关闭一次并检查关闭结果；
- 本测试使用精确读取契约，短读被包装函数判为失败，不表示所有 FatFs EOF 短读都属于 I/O 错误；
- `main.c` 关闭写文件后重开读取，容量允许时写终止符，再按请求量 `memcmp`。

测试字符串为 `test sd card write and read!`：`strlen=28`，源数组容量为 29，目标数组容量为 33。正常完整成功时，requested、bw、br 和文件长度应为 `28/28/28/28`；这是预测值，尚非成功实测。

首次上板前学习者预测为 `34/0/0/28`，曾将修改输出量的对象答成函数名；经纠正后答出写入修改 `bw`、读取修改 `br`。仍需用新约束无提示复测，不能把跟做完整代码计为独立实现。

## 实验过程及证据边界

1. 用户确认 SD 已备份、为 FAT32，板卡/SD/JTAG/串口已就绪。串口设置按既有步骤为 115200、8N1、无流控。
2. 安全工程构建成功，首次在 `f_open(write)` 返回 3。断电复插卡后仍相同；用户报告更换另一张卡后“还是一样”。第二张卡的型号、容量和独立读卡器检验没有完整记录，因此只降低单卡故障的优先级，不绝对排除兼容性或卡问题。
3. 勾选系统复位后再次观察到相同错误；这降低残留控制器状态的可能性，不证明所有时钟/MIO/外部线路正常。
4. 逐层添加 `disk_status`、`CfgInitialize`、`CardInitialize`、`IdentifyCard`、`SdCardInitialize` 和命令失败打印，定位到卡初始化流程。
5. 原理图第 4/11 页显示 Bank 501 与 U6 的 PS 侧使用 2.5 V，卡侧使用 3.3 V；工程原配置却为 LVCMOS33。用户 Save As 创建独立 `SD_TEST_SAFE_HW`，只将 Bank 1 改为 LVCMOS25，Bank 0 保持 LVCMOS33。
6. Block Design 验证及 Output Products 生成成功；导出 HDF 时未包含 bitstream。该改动依靠 PS 初始化参数生效，未重新生成或配置 PL bitstream。
7. 本地核对：原工程 BD 的 MIO40/46 保留 LVCMOS33，副本为 LVCMOS25；导出 HDF 和平台 `_2` 的 MIO40 初始化值由 `0x1680` 变成 `0x1480`（IO_Type 从 3 变 2）。这些是文件配置证据，不替代物理电压测量。

### “平台 _2 没反应”的独立原因

SDK 日志显示，部分切平台后的会话仅执行 `stop/ps7_init/ps7_post_config`，Application 为空、`no_download=true`，没有 `dow/con`。AI 早期把这些会话当成有效复测，随后已校正。不能把修改界面或执行初始化本身当作应用运行证据。

- 约 00:55:05 的原版运行使用平台 `_0` 并下载 `SD_TEST.elf`。
- 约 00:55:34 的平台 `_2` 会话仅初始化，没有下载或启动 ELF。
- 补齐 Application 与第一核 Download 后，约 01:01:04、01:01:42 的日志确认平台 `_2` 初始化、原版 ELF 下载及 `con`。
- 约 01:03:37～01:03:38 的日志确认平台 `_2` 初始化、安全 ELF 下载及 `con`。最终诊断以下面这次有效运行为准。
- 日志来自本地 `SD_TEST_SAFE_HW/SD_TEST_SAFE_HW.sdk/SDK.log`；下面的摘要和串口文本随本学习记录保存，未整包提交 SDK 元数据。

### 平台 _2 下安全程序的有效串口输出

```text
DIAG: disk_status=0x01

DIAG: CfgInitialize=0
DIAG: CMD=1 raw=0x00000100 status=2
DIAG: IdentifyCard=0
DIAG: CMD=8 raw=0x00000800 status=2
DIAG: CMD=55 raw=0x00003700 status=2
DIAG: SdCardInitialize=1
DIAG: CardInitialize=1
ERROR: f_open(write) returned 3

ERROR: safe write failed
```

解释必须保持以下边界：

- `FRESULT=3` 是 `FR_NOT_READY`；`DSTATUS=0x01` 是 `STA_NOINIT`，没有 `STA_NODISK`，表示驱动当时认为卡已插入、磁盘尚未初始化。
- `CfgInitialize=0` 表示主控制器初始化函数返回成功，不证明外部 CLK/CMD 已正确到达卡。
- `XSdPs_IdentifyCard` 会在 CMD1 不成功时选择 SD 分支并最终返回成功。CMD0 不要求响应，CMD1 超时可能属于正常探测；`IdentifyCard=0` 不证明卡已有效响应。
- `XSdPs_CmdTransfer` 的 `status=2` 为 `XSDPS_CT_ERROR`，本次 CMD8 超时后继续到 CMD55；CMD55 超时使 `XSdPs_SdCardInitialize` 失败，再上返 `CardInitialize=1` 和 `FR_NOT_READY`。
- 因果链：卡检测有信号 → 主控制器配置返回成功 → 命令响应超时 → 磁盘保留未初始化 → `f_open` 失败 → 安全应用报告失败。
- 当前有电源、接触/连通、MIO/时钟、U6 电平转换、卡与驱动交互等候选；没有波形和 U6 供电测量，尚不能确定根因或判定硬件损坏。

### 原版 SD_TEST 对照（用户明确要求）

在副本工作区中构建原应用时，先发现历史头文件路径 `E:/S9/...` 失效。已修正为 `../../SD_TEST_bsp/ps7_cortexa9_0/include`；链接库路径为 `../../SD_TEST_bsp/ps7_cortexa9_0/lib`，链接参数为：

```text
-Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,--end-group
```

原版构建成功。平台 `_2` 下有效运行仍输出：

```text
success to open SD Card~
ERROR : f_open returned 3;
256
fail to write SD Card~
ERROR:f_open returned 3;
fail to read SD Card~
€
```

- `success to open` 仅来自延迟 `f_mount(...,0)`，不表示已读卡。
- `256` 是固定计算的写请求量，不是实际成功写入量；原代码从容量为 29 的源数组请求 256 字节，仍存在越界读取风险，不应当作之后反复验证的正常测试程序。
- 读失败后继续 `%s` 打印未初始化的 `dest_str`，`€` 不是卡内数据或传输成功证据。
- 对照说明失败并非安全封装独有；未完成文件创建、成功写入、回读、文件长度和内容一致性验收。
- 上述路径修正只针对 `SD_TEST_SAFE_HW` 中的副本；未把原 `P03_SD_TEST` 的应用源码或 BD 当作修复目标。

## 万用表证据

用户提供了板卡照片，板上可辨认 U6、卡槽及 C154/C155；确认有万用表。依照原理图先断电确认 C154 与卡槽金属外壳的连通端，再在直流电压档上电测量：

| 测量 | 用户报告 | 能支持的结论 |
|---|---|---|
| C154 靠板边缘的一端对卡槽外壳，按断电通断步骤操作 | 原话：“靠近边缘侧响” | 该焊盘与外壳连通，结合原理图作为接地参考；未另报电阻数值 |
| C154 另一端对卡槽外壳，上电直流电压档 | 3.34 V | SD 卡 3.3 V 供电测点电压正常 |

读数是用户现场报告，未提供万用表屏幕照片；不升级为全板供电或动态纹波已验证。U6 的 C150～C153 测点、2.5 V/3.3 V 供电以及 CLK/CMD 连通性和波形均未测。最后指导为测完移开红表笔并断电；用户未单独确认最终电源状态，下次必须先确认。

## 临时驱动诊断的复现要点

仅在安全 BSP 的 `xilffs_v3_7/src/diskio.c` 中，在 `XSdPs_CfgInitialize` 和 `XSdPs_CardInitialize` 调用后分别打印返回值；在 `sdps_v3_3/src/xsdps.c` 的 `XSdPs_CardInitialize` 中，在 IdentifyCard、SdCardInitialize 调用后打印返回值。在 `XSdPs_CmdTransfer` 自身的返回标签处加入：

```c
RETURN_PATH:
    if (Status != XST_SUCCESS) {
        xil_printf("DIAG: CMD=%d raw=0x%08x status=%d\r\n",
                   (int)((Cmd >> 8U) & 0x3FU),
                   (unsigned int)Cmd,
                   (int)Status);
    }
    return Status;
```

`SD_Init` 用 `disk_status(0)` 打印初始化前状态，仍使用 `f_mount(&fatfs, "", 0)`。诊断只能看返回码，未插入绕过错误或强制成功的逻辑。先构建 BSP 再构建应用；重新生成 BSP 可能覆盖这些临时打印。

## 下一次只接一个主要实验

目标：定位并测量 U6 的供电，而非重复建工程或提前进入 P04。

1. 确认当前断电状态，获取 U6 及 C150～C153 的清晰近照，以实物和原理图确认测点；不根据模糊照片猜引脚。
2. 断电识别接地端，切换直流电压档后再上电测供电。C154 的 3.34 V 已记录；不要把通断档带电测量。
3. 若 U6 电源正常，再设计一个 CLK/CMD 连通或波形检查，区分“卡未供电”和“有供电但没有有效通信”等假设；万用表不能证明时钟频率、边沿或响应时序。
4. 再次上板前，短复测 `&bw/&br`、延迟挂载、IdentifyCard 返回值、SDK Download/ELF 选择。
5. 故障解决后运行安全应用，取得 requested/written/read=28、文件大小 28 和 `memcmp` 一致性证据，再做无提示最小重写、约束变化与延迟复测。当前成功路径及故障注入/恢复验收尚未完成。

## 来源与证据性质

- 本板原理图：仓库根目录 `AntMiner_ControlBoard_XC7010_V1.01控制板.pdf` 第 4、11 页；实物版本和测点对应仍以照片/现场核对为准。
- 匹配版本源代码：SDK BSP 中 `xilffs_v3_7/src/diskio.c`、`sdps_v3_3/src/xsdps.c`、`xsdps.h`，用于解释状态和命令路径。
- 配置事实：原/副本 `.bd`、新 HDF 与 `zynq_wrapper_hw_platform_2/ps7_init.tcl`，用于核对 MIO 配置。
- 实际运行：用户串口文本与 SDK.log 中初始化、ELF 下载、继续执行记录。
- 实际电压：用户报告 C154=3.34 V。硬件根因、U6 供电、成功块传输和文件回读仍为待验证事项。
