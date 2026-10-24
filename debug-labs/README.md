# FreeCAD 非界面层调试实验（C++ 原生驱动版）

围绕“一个长方体”，用**纯 C++ 调用**驱动 FreeCAD 的 Base/App 两层核心机制，
实验全程不写、不跑任何 Python 脚本（FreeCAD 内嵌的解释器只是底层管道）。
配套学习资料 FC01~FC10 分册；断点行号在 `releases/FreeCAD-1-1` 分支核实，
并已通过 FreeCADLabs 程序实跑验证。

## 用法

1. 设断点：按下面场景表在 `src/` 下打断点（热点函数记得用条件断点）；
2. F5 选 **“FreeCAD Labs (C++)”** 配置（目标 `build/debug/bin/FreeCADLabs.exe`）；
3. 集成终端里按回车推进场景，每个“回车检查点”= 回 VS Code 补设下一组断点的时机；
4. 改了 `cpp/LabsMain.cpp` 后：`pixi run build-debug`（增量，很快）再 F5；
   只编本目标：`pixi exec cmake --build build/debug --target FreeCADLabs`；
5. 无人值守验证：命令行 `FreeCADLabs.exe auto` 自动跑完全部场景。

接线：根 CMakeLists.txt 的 `option(FREECAD_BUILD_DEBUG_LABS ON)` 挂载本目录；
运行时 PATH 必须含 `build/debug/Mod/Part`（隐式链接的 Part.pyd 靠它解析，
launch 配置已带，上游 tests/CMakeLists.txt 注明模块测试在 Windows 上正缺这步）。

## 场景总览（按长方体的数据生命周期）

| 场景 | 主题 | 分册 | 核心断点（src/ 下 文件:行） |
|---|---|---|---|
| 0 | 类型系统与按需加载 | FC01/FC05 | Base/Type.cpp:139 createType、:95 createInstanceByName、:108 importModule |
| 1 | 对象创建两条路径 + 统一访问 | FC01/FC04/FC03§5 | App/Document.cpp:3085 addObject |
| 2 | 属性主链（核心）：setValue 通知链 | FC03 | App/PropertyStandard.cpp:1020、App/Property.cpp:299/284、App/Document.cpp:792、App/Transactions.cpp:388、App/DocumentObject.cpp:889、Mod/Part/App/FeaturePartBox.cpp:228/54 |
| 3 | 引用属性与依赖图 | FC04/FC03§12 | App/PropertyLinks.cpp:730、App/DocumentObject.cpp:435 |
| 4 | 对象级 vs 文档级重算、表达式 | FC03§9/§13 | App/Document.cpp:2692/3008、App/PropertyExpressionEngine.cpp:692、App/DocumentObject.cpp:297、DocumentObject.cpp:1217 setExpression |
| 5 | 事务：撤销/重做 | FC09/FC03§15 | App/Document.cpp:157/405、App/Transactions.cpp:177/388、App/PropertyStandard.cpp:1076 Paste |
| 6 | 持久化：保存/读回 | FC02/FC03§14 | App/Document.cpp:1763/984/1938、App/PropertyStandard.cpp:1056/1061、Mod/Part/App/PropertyTopoShape.cpp:704 |

FC06 工作台 / FC07 命令按钮 / FC08 视图提供者属于界面层，不在本套实验内。

## 实跑验证中确认的三个“易错点”（也是实验的看点）

1. **类型登记不在 DLL 加载时**：场景 0 启动即查 `fromName("Part::Box")`
   得到 `BadType`——登记发生在模块初始化（createInstanceByName 按需加载）。
2. **原生 `setValue` 不做范围钳制**：场景 2 写入 -5 会原样生效，随后
   `execute` 报 "too small" 拒绝更新形状；钳制只存在于脚本路径 setPyObject。
3. **两级重算不是“算过就跳过”**：直接 `DocumentObject::recompute()` 不清
   Touch/Enforce，文档级重算会再执行一次对象；表达式只在文档级
   `_recomputeFeature` 里求值——所以赋值瞬间的体积用的是旧 Width。

## 附：仓库自带 GTest（另一个纯 C++ 入口）

`build/debug/bin/App_tests_run.exe`（还有 Part/Mesh 等 `*_tests_run`）是
GTest 程序，用 `--gtest_filter` 挑用例、配合断点调试；测试源码在
`tests/src/App/`（Document.cpp、Property.cpp、Expression.cpp……），
是现成的“官方版 C++ 实验”。

## 推进顺序

0 → 1 → 2（心脏，多花时间）→ 3 → 4 → 5 → 6。
每跑完一个场景，对照分册的自检问题回顾调用栈窗口里看到的层级。
