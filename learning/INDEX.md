# S9 Zynq 系统学习总入口

这套系统的目标不是“把四个例程讲得更细”，而是以 S9 XC7Z010 控制板为实验平台，系统建立可迁移的 FPGA、Zynq PS/PL、嵌入式软件、AXI/DMA、网络、RTOS 和 Linux 能力。

当前框架包含：

- 1 份完整能力地图：T/C/R/P/B/A/M/N/O/L/S 共 83 个能力节点；
- 9 个主线阶段、P00–P40 共 41 个核心项目；
- X01–X05 共 5 个进阶选修；
- 对标课程、官方文档、2017.4 代码入口和同板社区项目的资料目录；
- 掌握证据、误区、复习、迁移和风险控制机制。
- 一份可以直接复制提示词的 AI 配合学习指南。

现有 P01–P04 是阶段 0 的审计材料，不是课程目录，也不决定后续知识边界。

## 从这里开始

第一次进入本系统时按以下顺序：

1. 阅读 [`research/COMPARATIVE_STUDY.md`](./research/COMPARATIVE_STUDY.md)，了解路线为什么这样组织，而不是照搬某套开发板教程。
2. 阅读 [`research/BOARD_CAPABILITY_MATRIX.md`](./research/BOARD_CAPABILITY_MATRIX.md)，确认本板可以安全承载哪些实验，以及哪些参数仍需实测。
3. 浏览 [`CURRICULUM.md`](./CURRICULUM.md) 和 [`ROADMAP.md`](./ROADMAP.md)，先看能力依赖和阶段退出条件。
4. 查看 [`PROGRESS.md`](./PROGRESS.md)，再用 [`assessments/BASELINE.md`](./assessments/BASELINE.md) 做闭卷基线诊断。
5. 按 [`AI_STUDY_GUIDE.md`](./AI_STUDY_GUIDE.md) 调用 AI，从 [`PROJECT_BACKLOG.md`](./PROJECT_BACKLOG.md) 选择能补最低先修缺口的项目。
6. 每次学习按 [`LESSON_PROTOCOL.md`](./LESSON_PROTOCOL.md) 留下证据，并更新掌握度、误区和复习队列。

当前 P01 UART 与 P02 GPIO 的学习内容已经完成，下一步先做两项独立复测，再进入 **P03 SD/FatFs 安全审计**。P00 中尚未形成证据的恢复和实验方法仍需补齐。若基线诊断显示某项已有独立迁移能力，可以凭证据跳过，而不是凭“看过”跳过。

## 系统结构

```text
在线调研 + 本板证据
  research/RESOURCE_CATALOG
  research/COMPARATIVE_STUDY
  research/BOARD_CAPABILITY_MATRIX
                |
                v
完整能力地图 CURRICULUM
                |
                v
阶段路线 ROADMAP -> 具体项目 PROJECT_BACKLOG
                |                    |
                v                    v
       LESSON_PROTOCOL       实验/源码/日志/波形
                \                    /
                 v                  v
         MASTERY + MISCONCEPTIONS + REVIEW_QUEUE
                 |
                 v
        延迟复测、近迁移、远迁移、毕业项目
```

资料决定“事实和覆盖面”，能力地图决定“要学什么”，项目库决定“如何在本板上取得证据”，掌握账本决定“下一步学什么”。这四层不能互相替代。

## 文件导航

### 调研与边界

| 文件 | 回答的问题 |
|---|---|
| [`research/RESOURCE_CATALOG.md`](./research/RESOURCE_CATALOG.md) | 已找到哪些教程、代码和权威资料？精确版本和取用规则是什么？ |
| [`research/COMPARATIVE_STUDY.md`](./research/COMPARATIVE_STUDY.md) | AX7010、Zynq Book、AMD 教程、Zybo 和 S9 社区项目分别贡献什么？ |
| [`research/BOARD_CAPABILITY_MATRIX.md`](./research/BOARD_CAPABILITY_MATRIX.md) | 板上有哪些资源、证据是否一致、实验风险是什么？ |
| [`SOURCES.md`](./SOURCES.md) | 来源优先级、版本锁定、许可证和引用策略是什么？ |
| [`PROJECT_MAP.md`](./PROJECT_MAP.md) | 当前四个工程的真实入口、问题和证据权重是什么？ |

### 学习设计

| 文件 | 回答的问题 |
|---|---|
| [`CURRICULUM.md`](./CURRICULUM.md) | 从基础到系统设计需要哪些能力，先修关系是什么？ |
| [`ROADMAP.md`](./ROADMAP.md) | 各阶段按什么顺序推进，达到什么证据才能退出？ |
| [`PROJECT_BACKLOG.md`](./PROJECT_BACKLOG.md) | P00–P40/X01–X05 各做什么、参考什么、怎样迁移和故障注入？ |
| [`LESSON_PROTOCOL.md`](./LESSON_PROTOCOL.md) | 一次讲解、实验和验收应该怎样进行？ |
| [`AI_STUDY_GUIDE.md`](./AI_STUDY_GUIDE.md) | 怎样向 AI 发起诊断、讲解、调试、写代码和验收？ |
| [`S9_ZYNQ_SYSTEMATIC_LEARNING_PLAN.md`](../S9_ZYNQ_SYSTEMATIC_LEARNING_PLAN.md) | 已有长篇讲解和实验草案在哪里？ |

### 诊断与进度

| 文件 | 回答的问题 |
|---|---|
| [`PROGRESS.md`](./PROGRESS.md) | 哪些项目内容已经完成，当前准备进入哪一项？ |
| [`assessments/BASELINE.md`](./assessments/BASELINE.md) | 第一轮闭卷诊断问什么？ |
| [`MASTERY.md`](./MASTERY.md) | 哪些能力有独立证据，哪些只是看过或跑通过？ |
| [`MISCONCEPTIONS.md`](./MISCONCEPTIONS.md) | 当前错误模型是什么，用什么实验纠正？ |
| [`REVIEW_QUEUE.md`](./REVIEW_QUEUE.md) | 现在最该复测什么，何时复测？ |
| [`transfer-challenges/P01_UART.md`](./transfer-challenges/P01_UART.md) | 如何把一个例程改造成近迁移和远迁移题？ |

## 九阶段主线

| 阶段 | 项目 | 核心产出 |
|---|---|---|
| 0 基线 | P00–P04 | 可恢复环境、工程解剖、四例程风险审计 |
| 1 RTL | P05–P08 | Verilog、Testbench、XDC、STA、复位、CDC、ILA |
| 2 PS 裸机 | P09–P13 | 启动到 main、MMIO、Timer、GIC、非阻塞事件系统 |
| 3 存储/启动 | P14–P16 | 安全 FatFs、FSBL/BIF、可复现 SD 冷启动 |
| 4 PS–PL 控制面 | P17–P22 | MIO/EMIO/AXI、寄存器 IP、中断、BRAM |
| 5 数据面 | P23–P27 | AXI-Stream、DMA、Cache、SG 和性能测量 |
| 6 通信/实时 | P28–P30 | PHY/GEM/lwIP、FreeRTOS 和过载恢复 |
| 7 Linux | P31–P35 | 启动链、设备树、用户态访问、platform/IRQ/DMA 驱动 |
| 8 本板系统 | P36–P40 | 自检、风扇、多通道串行、网络管线和毕业项目 |

详细先修和退出证据以 `ROADMAP.md` 与 `PROJECT_BACKLOG.md` 为准。

## AI 导师的工作约束

项目级导师 skill 位于 [`.agents/skills/s9-zynq-mentor/SKILL.md`](../.agents/skills/s9-zynq-mentor/SKILL.md)。在 `S9miner_sample` 目录使用 `$s9-zynq-mentor` 时，AI 应当：

1. 先读取本入口、掌握度和对应项目，不凭例程编号自动安排下一课；
2. 新主题先查本板证据、匹配 2017.4 的官方代码和权威文档，再借鉴完整课程；
3. 从物理信号、时钟/复位、寄存器/协议、CPU/Cache、驱动、应用一路解释到可观测结果；
4. 明确区分实测事实、文档事实和待验证推断；
5. 每次增加约束变化、故障注入和迁移题，不把逐行翻译或成功运行当作掌握；
6. 只有出现独立证据时才更新 `MASTERY.md`。

## 资料和代码如何进入项目

调研阶段不把大型第三方仓库整包复制进来。开始某个项目时，才从 `research/RESOURCE_CATALOG.md` 指定的版本取最小必要代码，并记录：

- 原始 URL、标签或提交；
- 原路径和许可证；
- 与本板器件、DDR、MIO、XDC、时钟、地址和外设的差异；
- 哪些部分由学习者独立重写；
- 编译、运行和迁移证据。

这样既能利用成熟资料，又不会把“复制别人的完整工程”误当成迁移能力。

## 完成的定义

一个知识节点只有在留下最小因果模型、权威来源、安全实验、故障定位、约束变化迁移和延迟复测后，才算真正完成。跑通现成例程最多说明接触过；能够独立重建、解释失败并迁移到另一接口或开发板，才是本系统追求的掌握。
