# Git 提交格式规范（规则 1）

## 格式

```
[TAG] 简洁描述
```

## TAG 范围

| TAG | 用途 |
| --- | --- |
| `[FEAT]` | 新功能、新模块 |
| `[FIX]` | Bug 修复 |
| `[DOCS]` | 文档变更 |
| `[STYLE]` | 代码格式、命名（无逻辑变化） |
| `[REFACTOR]` | 重构（不改功能） |
| `[TEST]` | 测试代码 |
| `[CHORE]` | 构建脚本、依赖、工具链 |
| `[PERF]` | 性能优化 |
| `[BREAKING]` | 破坏性变更（接口不兼容） |

## 规则

1. **用户确认后才提交** — 任何 `git commit` 前必须先展示提交信息给用户确认
2. **禁止自动签名** — 禁止 `Made-with: Cursor`、`Co-authored-by: AI` 等自动追加内容
3. **描述简洁明了** — 用祈使句，说明做了什么，不超过 72 字符
4. **渐进迁移各阶段独立 commit** — 见 `progressive-migration.md`

## 示例

```
[FEAT] add uart-v2 DMA backend skeleton
[FIX] fix SPI clock polarity in BSP rt1176
[DOCS] establish new docs/rules constraint system
[REFACTOR] remove old uart module after v2 verification
[CHORE] update cmake-rules.md path references
[BREAKING] change drv_open() signature to return status code
```

## 违规后果

- 提交被 review 拒绝
- 历史 log 污染，问题追溯困难
