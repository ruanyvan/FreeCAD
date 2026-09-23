# pixi-fix: VS 2026 MSVC 激活兜底

## 这是什么

这台机器上只安装了 Visual Studio 2026（v18），而 FreeCAD 1.1 的 pixi 环境
（`pixi.lock` 锁定版本）依赖的 conda 包 `vs2022_win-64`（构建号 `_33`）
找不到它：它的 vswhere 查询（`-version [17.0,18.0)`、组件 ID）和硬编码的
VS2022 路径全部落空。结果其激活脚本不执行 vcvars，却仍设置了
`CC=cl.exe`，CMake 因此报错：

```
Could not find compiler set in environment variable CC: cl.exe
```

`vs2026_fallback_activate.bat` 是修复脚本：按字母序在
`vs2022_compiler_vars.bat` 之后运行，用 `vswhere -latest` 找到 VS 2026，
清除前者留下的 `VSINSTALLDIR` 等污染变量，再以 SxS 注册名
（`-vcvars_ver=14.44.17.14`）调用 vcvars64.bat，最终选中与 conda 包
完全一致的 MSVC 工具集 14.44.35207。

## 使用方法

该脚本必须存在于
`.pixi/envs/default/etc/conda/activate.d/` 中才会生效。**pixi 环境一旦
重建（`pixi clean`、pixi.lock 变更、手动删除 `.pixi` 等）就会丢失**，
此时双击运行本目录下的：

```
restore-msvc-fix.bat
```

它会重新复制脚本并自动验证 `cl.exe` 是否可用。

## 验证

```powershell
pixi run where cl.exe
# 期望输出: ...\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe
```

## 文件清单

| 文件 | 作用 |
|---|---|
| `vs2026_fallback_activate.bat` | 兜底激活脚本（备份原件） |
| `restore-msvc-fix.bat` | 一键恢复 + 验证 |
