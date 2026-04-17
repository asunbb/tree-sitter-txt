--- litetxt.nvim — tree-sitter txt 语法插件的 Lua 模块
--- 将缩进文本解析器（litetxt）注册为 nvim-treesitter 本地 parser 源，
--- 并把 Neovim 的 text filetype 映射到该 parser。

local M = {}

--- 获取插件根目录的绝对路径
--- 利用 Lua 的 debug.getinfo 获取当前文件路径，再向上三级目录得到插件根。
--- 路径推算：init.lua → litetxt/ → lua/ → <插件根目录>
---@return string 插件根目录的绝对路径
function M.get_root()
  -- debug.getinfo(1, "S").source 返回 "@<文件绝对路径>"，去掉首字符 "@" 后得到纯路径
  local source = debug.getinfo(1, "S").source:sub(2)
  -- :p:h:h:h 依次执行：转为全路径(:p) → 去掉文件名(:h) → 去掉 lua/(:h) → 去掉 litetxt/(:h)
  local path = vim.fn.fnamemodify(source, ":p:h:h:h")
  print("litetxt -> " .. path)
  return path
end

--- 插件初始化入口
--- 1. 注册 TSUpdate autocommand，在 nvim-treesitter 安装时注入 litetxt parser 信息
--- 2. 将 Neovim 的 text filetype 映射到 litetxt parser（Neovim 默认把 .txt 识别为 text 类型）
---@param opts table|nil 预留配置项（暂未使用）
function M.setup(opts)
  opts = opts or {}

  -- nvim-treesitter 的 install/update 流程会先调用 reload_parsers() 清除模块缓存，
  -- 然后才触发 User TSUpdate 事件（见 nvim-treesitter install.lua:491-496）。
  -- 因此直接注册 parsers 表无效（会被清除），必须用 autocommand 在 TSUpdate 事件中注入。
  -- require 放在回调内部，确保此时 nvim-treesitter 已加载。
  vim.api.nvim_create_autocmd("User", {
    pattern = "TSUpdate",
    callback = function()
      local ok, ts_parsers = pcall(require, "nvim-treesitter.parsers")
      if not ok then
        return
      end
      ts_parsers.litetxt = {
        install_info = {
          -- 本地 parser 路径，使用插件根目录（包含 grammar.js、src/ 等文件）
          path = M.get_root(),
          -- generate = true：需要从 grammar.js 生成 parser.c（因为仓库不提交预编译产物）
          generate = true,
          -- generate_from_json = false：不使用 grammar.json 生成，而是从 grammar.js 完整生成
          generate_from_json = false,
          -- queries：指定查询文件目录（相对于插件根目录），
          -- Neovim 通过 runtimepath 的 queries/<language>/*.scm 发现查询
          queries = "queries",
        },
      }
    end,
  })

  -- Neovim 默认把 .txt 文件识别为 "text" filetype，
  -- 而我们的 parser 名为 "litetxt"，需要通过 register 建立映射关系
  vim.treesitter.language.register("litetxt", { "text" })
end

return M
