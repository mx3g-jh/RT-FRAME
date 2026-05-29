# 项目约束规范体系

本目录是项目的**强约束文档中心**，所有规则、计划、规范均以此为准。

## 文档索引

| 文件 | 内容 |
| ---- | ---- |
| [constraints.md](constraints.md) | **所有 10 条强约束**的完整定义与详细解释（从这里开始读） |
| [progressive-migration.md](progressive-migration.md) | 规则 10：渐进迁移开发流程四阶段规范 |
| [layer-usage.md](layer-usage.md) | 规则 6：分层架构期望状态与使用规范 |
| [cmake-rules.md](cmake-rules.md) | 规则 5：CMake 层隔离抽象函数规范 |
| [git-commit.md](git-commit.md) | 规则 1：Git 提交格式规范 |
| [plans/SUMMARY.md](plans/SUMMARY.md) | 所有计划的汇总索引（状态总览） |

## 快速导航

- **不知道能不能直接动手？** → 读 [constraints.md](constraints.md) 规则 2
- **要重构/新增功能？** → 读 [progressive-migration.md](progressive-migration.md)
- **写 CMakeLists 遇到问题？** → 读 [cmake-rules.md](cmake-rules.md)
- **要提交代码？** → 读 [git-commit.md](git-commit.md)
- **查计划状态？** → 看 [plans/SUMMARY.md](plans/SUMMARY.md)

## 约束体系设计原则

1. **文档即约束**：规则只在本目录下生效，旧文档（`docs/claude/`）已废弃
2. **独立可读**：每份文档可独立阅读，不需要跳转其他文档才能理解
3. **期望与现状分离**：未完成的重构明确标注 TODO，不误导为已定稿规范
4. **新建计划必须登记**：所有新计划在 [plans/SUMMARY.md](plans/SUMMARY.md) 登记后才能开工
