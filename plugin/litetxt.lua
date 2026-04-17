--- litetxt.nvim 自动加载入口
--- Neovim 启动时会自动加载 runtimepath 上 plugin/ 目录下的所有 Lua 文件，
--- 因此此文件无需用户手动 require，插件安装后即自动生效。

-- 防重复加载：当多个插件管理器或配置片段重复引入时，
-- 确保 setup() 只执行一次，避免重复注册 autocommand 和 filetype 映射

local _g = vim.g

local litetxt = _g.litetxt
if litetxt ~= nil then
  return
end
_g.litetxt = {}

require("litetxt").setup()

