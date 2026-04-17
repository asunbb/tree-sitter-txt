# tree-sitter-txt-again

[English](README.md)

一个 [tree-sitter](https://tree-sitter.github.io/) 语法项目（语法名 **litetxt**），用于解析缩进文本文件，为编辑器中的 `.txt` 文件提供代码折叠功能。

## 功能特性

- **基于缩进的段落解析** — 不同缩进层级的内容被结构为父子段落
- **多级嵌套** — 支持任意深度嵌套
- **不规则缩进处理** — 正确处理代码块等不规则缩进模式
- **编辑器折叠支持** — 包含折叠查询（`queries/folds.scm`），适用于 Neovim、VS Code 等兼容 tree-sitter 的编辑器

## 快速开始

### 前置依赖

- [Node.js](https://nodejs.org/)
- [tree-sitter CLI](https://tree-sitter.github.io/tree-sitter/creating-parsers#installation)（`tree-sitter-cli`）

### 生成解析器

```bash
npm run generate
```

### 运行测试

```bash
npm test
```

### 从标准输入解析文本

```bash
echo "标题
  子项 1
  子项 2
    嵌套细节" | npm run parse
```

## 语法规则

语法定义了三条核心规则：

| 规则 | 说明 |
|------|------|
| `document` | 根节点，包含零或多个 segment |
| `segment` | 一行内容，可选地后跟缩进的子 segment |
| `content` | 匹配 `/[^\n]+/` 的原始文本 |

缩进处理通过 `src/scanner.c` 中的外部 token 实现：

| Token | 用途 |
|-------|------|
| `_newline` | 同级段落之间的换行 |
| `_indent` | 缩进层级增加 |
| `_dedent` | 缩进层级减少 |

## Neovim 安装

### lazy.nvim

```lua
{
  "asunbb/tree-sitter-txt",
  dependencies = {"nvim-treesitter/nvim-treesitter"},
  config = function()
    require("litetxt").setup()
  end,
}
```

安装后，打开 `.txt` 文件并执行 `:TSInstall litetxt` 编译 parser。

## 项目结构

```
grammar.js              语法定义
tree-sitter.json        tree-sitter 元数据
package.json            npm 包配置
lua/litetxt/init.lua    Neovim 插件核心逻辑
plugin/litetxt.lua      Neovim 自动加载入口
queries/litetxt/
  folds.scm             编辑器折叠查询
src/
  parser.c              生成的解析器
  scanner.c             缩进外部扫描器
  node-types.json       生成的节点类型定义
  grammar.json          生成的语法 AST
test/corpus/basic.txt   测试用例
```

## 许可证

MIT

## 免责声明

- 本项目代码主要由 coding agent（AI）生成。
- 取用由人，使用者自行判断和负责。
- 因使用本软件导致的任何数据丢失，作者不承担责任。
