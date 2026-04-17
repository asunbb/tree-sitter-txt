# tree-sitter 缩进解析器设计原则

## 规则 1：DEDENT 不能吞掉父级需要的换行符

### 问题

当 scanner 检测到缩进回退时，DEDENT token 消费了 `\n` + 下一行前导空格。
之后 parser 回到父级上下文，期望用 NEWLINE 连接下一个兄弟节点，
但换行符已被 DEDENT 吃掉，scanner 无法再发出 NEWLINE，导致解析错误。

根本原因：同一个 `\n` 既要充当 DEDENT 的边界，又要充当父级 NEWLINE 的分隔符。
单字符只能产一个 token，不补发就会断裂。

### 解决方案

Scanner 维护 `newline_after_dedent` 布尔标志：

1. DEDENT 消费 `\n` + 空格后，设置标志为 true
2. 下次 scan 调用时，若 NEWLINE 在当前 parser 上下文中合法，补发零宽度 NEWLINE
3. 若 NEWLINE 不合法（如已回到 document 级别，`repeat(segment)` 不需要 NEWLINE），清除标志

### 受影响的模式

```
Parent
  Child A
    Grandchild
  Child B        ← 从 level 3 回退到 level 2，仅回退 1 级
```

回退 2+ 级到 document 级别时不受影响（`repeat(segment)` 不需要 NEWLINE）。

---

## 规则 2：必须用缩进栈追踪实际嵌套层数

### 问题

旧实现用级别差计算 DEDENT 数量：

```c
pending_dedents = indent_level - new_level - 1;
```

这假设缩进每次均匀递增 2 空格（= 1 级）。但嵌入代码块的缩进是任意空格数：

```
        * 原文章说明                    (12 空格, level 6)
          float func(...) {            (21 空格, level 10)
                float deno = ...;       (37 空格, level 18)
                return 1.0 / ...;      (27 空格, level 13)
          }                            (10 空格, level 5)
          参数说明                      (15 空格, level 7)
```

从 level 13 → level 5，旧代码算出 `pending_dedents = 13 - 5 - 1 = 7`，发出 8 个 DEDENT。
但实际只有 2 层 INDENT（level 6→10, level 10→18），只需要 2 个 DEDENT。
多余的 DEDENT 把父级的 block 也关掉了，导致后续行跑到错误的层级。

### 解决方案

Scanner 维护 `indent_stack[]` 数组：

- **INDENT 时**：push 当前级别到栈顶
- **DEDENT 时**：从栈顶 pop 到目标级别，pop 次数 = 实际需要关闭的层数

```c
uint32_t pops = 0;
while (stack_size > 1 && indent_stack[stack_size - 1] > new_level) {
    stack_size--;
    pops++;
}
pending_dedents = pops - 1;  // 首个 DEDENT 立即发出
```

### 为什么不能只用一个 indent_level 计数器

计数器只记录当前绝对级别，丢失了"从哪个级别 INDENT 过来"的历史信息。
栈记录了每次 INDENT 的目标级别，回退时才能精确关闭正确的层数。

---

## 必须覆盖的测试模式

| 模式 | 描述 | 覆盖的规则 |
|------|------|-----------|
| 回退 1 级到兄弟节点 | level 3 → level 2，兄弟在同一父级下 | 规则 1 |
| 不规则缩进（代码块） | INDENT 跨多级（如 level 3→8），DEDENT 只需关闭 1 层 | 规则 2 |
| 多层嵌套 + 回退到 document | 验证 pending_dedents + newline_after_dedent 配合 | 规则 1 + 2 |

