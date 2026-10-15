# P01 UART 阻塞故障注入证据

- 日期：2026-09-21
- 能力节点：P02 UART、C01 指针与容量、T04 实验方法
- 辅助程度：部分脚手架。AI 完成源码备份、故障注入、恢复和构建时间戳诊断；学习者完成闭卷预测、SDK 构建/下载和上板观测。
- 工程锚点：`P01_HELLO/HELLO.sdk/HELLO/src/helloworld.c`
- 正常基线备份：`learning/evidence/P01_UART/helloworld_before_blocking_fault_2026-09-21.c`
- 故障版本：`learning/evidence/P01_UART/helloworld_blocking_fault_2026-09-21.c`

## 预测与诊断

- 正确预测：无条件调用 `inbyte()` 且没有输入时，执行流会停在接收等待中。
- 首次闭卷暴露：把当前轮询路径误说成 UART 中断；把 `count` 误说成外设寄存器值；混淆 `p`、`*p` 和 `&p`。
- 纠正后判断：250 ms 轮询的最坏响应延迟约为 250 ms，名义打印周期仍为 1 s，主循环约 4 次/秒；本 BSP 的 `usleep()` 使用 Global Timer 忙等，CPU 没有真正休眠。

## 上板观测

### 正常基线

无输入时周期输出持续递增；在 `count = 18` 后发送 `i`，得到 `Current count = 19`，随后周期输出继续从 `count = 19` 递增。

### 故障注入

绕过 `XUartPs_IsReceiveData()` 保护、无条件调用 `inbyte()` 后，启动只输出：

```text
Hello Zynq, count = 0
```

之后没有周期输出。每发送一个无换行的 `i`，只出现：

```text
i
Current count = 1
```

连续多次发送 `i` 时，`Current count` 始终为 1。每个字符只允许主循环前进一轮，`elapsed_ms` 每轮仅累计 10 ms，尚未达到 1000 ms。

### 恢复验证

恢复接收状态保护、重新编译并下载 ELF 后，无输入时重新出现：

```text
Hello Zynq, count = 0
Hello Zynq, count = 1
Hello Zynq, count = 2
Hello Zynq, count = 3
Hello Zynq, count = 4
Hello Zynq, count = 5
Hello Zynq, count = 6
```

恢复过程中还发现：直接复制旧备份会保留较早的修改时间，SDK 增量构建可能复用故障版 `.o/.elf`。刷新源码时间戳并重新 Build 后，正常 ELF 才实际生成并下载。

## 结论

- 已用实际现象验证 `inbyte()`/`XUartPs_RecvByte()` 的阻塞会停止同一执行流中的周期任务。
- 已验证只修改 C 应用时仅需重建和下载 ELF，不需要重新生成 bitstream。
- 本次代码操作由 AI 完成，且闭卷路径与指针题经过纠正提示，因此只能加强 L1 证据，不能支持 L2。
- 延迟复测：隔日不看源码重画轮询数据路径，并独立解释 `p`、`*p`、`&p`；随后无参考重写最小 `i/r/q/c` 命令循环。