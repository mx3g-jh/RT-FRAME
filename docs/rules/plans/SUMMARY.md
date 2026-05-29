# 计划汇总索引

本文是所有计划的统一入口。新建计划必须在此登记，完成或改版必须回填状态。

---

## 活跃计划

| 计划文件 | 标题 | 创建日期 | 最后更新 | 状态 |
| -------- | ---- | -------- | -------- | ---- |
| （新计划在此登记） | - | - | - | 🔄 进行中 |

---

## 已完成计划（历史归档，只读）

以下计划存于 `docs/claude/plans/`，内容只读，状态已确认完成：

| 计划文件 | 标题 | 完成日期 |
| -------- | ---- | -------- |
| [googletest-integration.md](../../claude/plans/googletest-integration.md) | GoogleTest 集成 | 2026 |
| [platform-include-refactor.md](../../claude/plans/platform-include-refactor.md) | Platform Include 重构 | 2026 |
| [kconfig-integration.md](../../claude/plans/kconfig-integration.md) | Kconfig 集成 | 2026 |
| [driver-bsp-migration.md](../../claude/plans/driver-bsp-migration.md) | Driver-BSP 迁移 | 2026-05-26 |
| [sitl-and-pthread.md](../../claude/plans/sitl-and-pthread.md) | SITL & pthread | 2026 |
| [hrt-layered-reorg.md](../../claude/plans/hrt-layered-reorg.md) | HRT 分层重组 | 2026 |
| [itcm-hot-path.md](../../claude/plans/itcm-hot-path.md) | ITCM Hot Path | 2026 |
| [flexram-repartition.md](../../claude/plans/flexram-repartition.md) | FlexRAM 重分区 | 2026 |
| [rename-and-cleanup.md](../../claude/plans/rename-and-cleanup.md) | 重命名与清理 | 2026 |
| [lock-free-ringbuffer-plan.md](../../claude/plans/lock-free-ringbuffer-plan.md) | 无锁环形缓冲区 | 2026 |
| [uart-console-merge-lpuart10-plan.md](../../claude/plans/uart-console-merge-lpuart10-plan.md) | UART Console 合并 | 2026 |

---

## 进行中待办（对应 plans/ 中活跃计划）

| 待办文件 | 对应计划 | 状态 |
| -------- | -------- | ---- |
| [devicetree-uorb-plan](../../claude/todos/devicetree-uorb-plan.md) | devicetree-uorb-plan | 🔄 进行中 |
| [engineering-quality](../../claude/todos/engineering-quality.md) | engineering-quality | 🔄 进行中 |

---

## 使用说明

### 新建计划
1. 在 `docs/rules/plans/` 新建 `<topic>.md`（使用规则2模板）
2. 在本文"活跃计划"表中登记一行
3. 执行 commit：`[DOCS] add plan: <topic>`

### 完成计划
1. 将本文中该计划行移入"已完成计划"表，填写完成日期
2. 执行 commit：`[DOCS] mark plan done: <topic>`

### 改版计划
1. 直接编辑对应 `plans/<topic>.md`，在文件末尾追加"改版记录"章节
2. 更新本文中该计划行的"最后更新"字段
3. 执行 commit：`[DOCS] update plan: <topic>`
