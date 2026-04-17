# tree-sitter.json 字段说明

文件：`tree-sitter.json`

## 当前内容

```json
{
  "grammars": [
    {
      "name": "litetxt",
      "camelcase": "Litetxt",
      "scope": "source.txt",
      "file-types": [".txt"]
    }
  ],
  "metadata": {
    "version": "0.1.0",
    "license": "MIT",
    "description": "Tree-sitter grammar for indented text segments"
  },
  "bindings": {}
}
```

## 字段详解

### 顶层结构

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `grammars` | 数组 | 是 | 语法定义列表，支持在一个包中包含多个语法 |
| `metadata` | 对象 | 是 | 包的元数据（版本、许可证、描述） |
| `bindings` | 对象 | 否 | 各语言绑定的生成配置 |

### grammars[].name — `"litetxt"`

语法的唯一标识名，全小写。用途：
- 生成 C 函数前缀：`tree_sitter_litetxt`
- npm 包名：`tree-sitter-litetxt`
- 编辑器中的语言 ID

### grammars[].camelcase — `"Litetxt"`

大驼峰形式。用途：
- 生成 C 中的外部函数声明：`tree_sitter_Litetxt()`
- 与 `name` 的差异：`name` 用于包/文件名，`camelcase` 用于代码符号

### grammars[].scope — `"source.txt"`

编辑器中的语言 scope 名称。用途：
- Neovim、VS Code 等编辑器用此值关联语法高亮规则
- 格式为 `source.xxx`（代码类）或 `text.xxx`（标记语言类）
- 本项目使用 `source.txt`，表示纯文本源文件

### grammars[].file-types — `[".txt"]`

文件类型匹配规则，告诉编辑器和工具链哪些文件应该用此语法解析。

匹配方式：
- **带点号前缀**（如 `".txt"`）— 扩展名后缀匹配，所有以 `.txt` 结尾的文件
- **不带点号**（如 `"Makefile"`）— 完整文件名匹配

生效场景：
- `tree-sitter parse` 命令根据此字段自动选择语法
- 编辑器（Neovim/Helix/VS Code）自动关联文件到此语法
- 如果多个语法声明了相同 file-types，由编辑器按优先级或用户配置决定

变更记录：从 `[".txt", "text"]` 改为 `[".txt"]`。移除 `"text"` 是因为无扩展名的文件名匹配在实际场景中罕见且容易误匹配。

### metadata.version — `"0.1.0"`

语义化版本号（SemVer）。用于 npm 发布和 tree-sitter 注册表。

### metadata.license — `"MIT"`

开源许可证标识，使用 SPDX 标识符。

### metadata.description

一句话描述语法用途，用于 npm 包描述和 tree-sitter 注册表展示。

### bindings — `{}`

语言绑定配置，控制如何为 Python、Rust、Go 等生成绑定。当前为空对象，表示使用默认配置，不额外生成语言绑定。
