# P02 GPIO：读—改—写与并发课程记录

- 日期：2026-09-22
- 节点：P03 PS GPIO/MIO、C01 指针与容量（开场复测）
- 风险：S0；仅做源码追踪、闭卷问答和纸面故障注入，未修改工程或硬件配置
- 源码锚点：`P02_GPIO/GPIO.sdk/GPIO_bsp/ps7_cortexa9_0/libsrc/gpiops_v3_3/src/xgpiops.c` 中的 `XGpioPs_WritePin`、`XGpioPs_SetDirectionPin` 和 `XGpioPs_SetOutputEnablePin`

## 已观察证据

- UART 复测能说明 RX FIFO、状态轮询、`inbyte`、`command` 和 `handle_command` 的路径，并区分轮询与中断；经纠正后定位到 `XUartPs_RecvByte` 内的阻塞循环。
- 能区分 `(*p)++` 与 `*p++`；缓冲区容量与 `strlen` 的区别仍需延迟闭卷复测。
- GPIO70 的 Bank 2、bit 16 和 DATA_MSW 判断正确；GPIO70 Mask/Data 首答不完整，讲解低有效掩码后独立算出 GPIO71 清零值 `0xFFFD0000`。
- 能计算保留其他位的读—改—写：`0x05 | (1U << 3) = 0x0D`；OUTEN 从 `0x12` 同时置 bit 3、清 bit 4 得到 `0x0A`。
- 在并发交错题中理解旧快照写回会丢失中断更新，也判断 `volatile` 不能提供原子性；bit 掩码换算曾错误，提示后修正为 bit 3=`0x08`、bit 5=`0x20`、组合=`0x28`。
- 能复述“保存中断状态—禁止中断—读—修改—写—恢复状态”的最小临界区，但独立伪代码仍沿用上一题模板，没有一次完整迁移为 OUTEN 双位操作。

## 当前结论

- 内容状态：DIRM/OUTEN 读—改—写及并发风险已经学习。
- 掌握证据：引导式即时迁移，维持 L1，不提升到 L2。
- 下次复测：不看本记录，闭卷写出 OUTEN 或 DIRM 双位更新；解释为何只保护写操作不够、为何必须恢复旧中断状态；再次说明缓冲区 capacity 与字符串长度的区别。
