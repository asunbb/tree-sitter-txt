# tree-sitter DEDENT token 范围与折叠边界

## 规则 3：DEDENT token 的范围不能延伸到下一个兄弟节点

### 问题

当文档结构如下：

```
PBR核心理论与渲染原理
  线性空间 linear space
    aaaa
    sfdsa
    fsadfsa
    sdfafsafa
  1111111
  2222222
```

"线性空间" segment 的 AST 范围是 `[15, 4] - [23, 4]`，而 "1111111" 的起始位置恰好也是 `[23, 4]`。
在编辑器中折叠 "线性空间" 时，"1111111" 被错误地包含在折叠范围内。

### 根本原因

Scanner 在发出 DEDENT token 前已经消费了所有字符：

```
\n  →  空白行(跳过)  →  下一行的前导空格
```

Token 的"范围"（extent）由 `mark_end` 调用时的 lexer 位置决定。
如果不调用 `mark_end`，token 范围默认延伸到 lexer 的最终位置——即下一行内容之前。

由于 DEDENT 是 segment 规则中的**最后一个 token**：

```
segment := content, optional(indent, segment, repeat(newline, segment), DEDENT)
                                                                  ^^^^^^
                                                          最后一个 token
```

segment 节点的结束位置等于 DEDENT token 的结束位置。
DEDENT 延伸到哪里，segment 就延伸到哪里。

### 核心洞察

Scanner 必须消费前导空格才能确定缩进级别（决定发 NEWLINE/INDENT/DEDENT），
但**消费 ≠ 包含**。可以用 `mark_end` 在消费之前"画一条线"，
让 token 的范围停在 `'\n'` 之后、空格之前。

关键在于：不同 token 需要不同的范围策略：

| Token | mark_end 位置 | 原因 |
|-------|--------------|------|
| DEDENT | `'\n'` 之后（空格之前） | segment 最后一个 token，范围决定折叠边界 |
| NEWLINE | 消费完全部空格之后 | 不影响 segment 结束位置，不影响折叠 |
| INDENT | 消费完全部空格之后 | segment 内的第一个 token，不影响 segment 结束位置 |

### 解决方案

在 scanner.c 的 scan 函数中：

**步骤 1**：消费 `'\n'` 后立即调用 `mark_end`

```c
advance(lexer);          // 消费 '\n'
lexer->mark_end(lexer);  // 标记位置在空格之前
```

**步骤 2**：空白行循环后，根据 token 类型决定是否重新标记

```c
if (DEDENT) {
    // 不重新标记 — DEDENT 范围停在 '\n' 之后
    // segment 结束于 [next_row, 0]，不与下一个兄弟重叠
} else if (NEWLINE || INDENT) {
    lexer->mark_end(lexer);  // 重新标记到当前位置（空格之后）
}
```

**步骤 3**：增强 `newline_after_dedent` 处理器

DEDENT 的 mark_end 使 parser 停在 `[next_row, 0]`（空格之前）。
如果 `newline_after_dedent` 只发零宽度 NEWLINE，content 正则会把前导空格也吃进去。
所以处理器必须主动消费空格和空白行：

```c
if (scanner->newline_after_dedent) {
    if (valid_symbols[NEWLINE]) {
      scanner->newline_after_dedent = false;
      while (true) {
        while (lexer->lookahead == ' ') advance(lexer);
        if (lexer->lookahead == '\n') { advance(lexer); continue; }
        break;
      }
      lexer->mark_end(lexer);
      lexer->result_symbol = NEWLINE;
      return true;
    }
}
```

### 修复前后对比

```
修复前:  segment("线性空间")  [15, 4] - [23, 4]  ← 与 "1111111" [23, 4] 重叠
修复后:  segment("线性空间")  [15, 4] - [22, 0]  ← 结束于上一行末尾
         segment("1111111")  [23, 4] - [23, 11]  ← 不再被包含在折叠中
```

### 经验教训

1. **Token 范围 ≠ 消费范围**。Scanner 可以消费字符来确定 token 类型，
   但用 `mark_end` 控制 token 的实际范围。

2. **最后一个 token 决定节点边界**。在 tree-sitter 中，节点的范围由其子 token 的起止位置决定。
   如果 DEDENT 是 segment 规则的最后一个 token，它的范围就直接决定了折叠能看到多远。

3. **mark_end 需要条件化使用**。不同 token 对范围的要求不同。
   一刀切地在循环开头 mark_end 会破坏 NEWLINE/INDENT 的正常行为。
   正确做法是先统一标记，再按 token 类型选择性覆盖。

4. **修改 mark_end 策略时，必须同步修改 newline_after_dedent**。
   mark_end 提前了，parser 的位置也提前了，后续处理器必须补偿这个偏移量。

---

## 必须覆盖的测试模式（补充）

| 模式 | 描述 | 覆盖的规则 |
|------|------|-----------|
| DEDENT 无空白行 | segment 结束位置不与下一个兄弟重叠 | 规则 3 |
| DEDENT 有空白行 | 同上，空白行不影响边界 | 规则 3 |
| DEDENT 有尾部空格的空白行 | `    ` + `\n` 被正确跳过 | 规则 1 + 3 |
| 多级 DEDENT (pending_dedents) | 所有零宽度 DEDENT 都在正确位置 | 规则 2 + 3 |
