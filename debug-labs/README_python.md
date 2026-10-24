# FreeCAD 非界面层调试实验（控制台版）

围绕"一个长方体"的非界面层核心机制，配套学习资料 FC01~FC10 分册。
所有断点行号已在 `releases/FreeCAD-1-1` 分支逐一核实。

## 用法

1. VS Code 打开某个实验脚本，按脚本头部清单设好断点；
2. F5 选 **"FreeCADCmd 脚本实验 (运行当前文件)"**；
3. 脚本中的 `input()` 检查点 = 回 VS Code 补设下一组断点的时机；
4. 交互式探索用 **"FreeCADCmd (commandline)"** 配置（集成终端里出现 `>>>`）。

## 实验总览

| 实验 | 主题 | 对应分册 | 核心断点（文件:行） |
|---|---|---|---|
| lab1 | 类型系统 + Python 绑定 | FC01 / FC10 | DocumentPyImp.cpp:314、Type.cpp:95/108 |
| lab2 | 属性系统主链（最核心） | FC03 | PropertyStandard.cpp:1020、Property.cpp:284/299、FeaturePartBox.cpp:54/228 |
| lab3 | 对象模型与依赖图 | FC04 / FC03§12 | PropertyLinks.cpp:730、DocumentObject.cpp:435 |
| lab4 | 文档重算调度 + 表达式 | FC03§9/§13 | Document.cpp:2692/3008、PropertyExpressionEngine.cpp:692 |
| lab5 | 事务（撤销/重做） | FC09 | Transactions.cpp:388/177、Document.cpp:157、PropertyStandard.cpp:1076 |
| lab6 | 持久化（保存/恢复） | FC02 | Document.cpp:1763/1938、PropertyTopoShape.cpp:704 |

纯界面层主题（FC06 工作台、FC07 命令按钮、FC08 视图提供者）不在本套实验范围，
需要时用 "FreeCAD (GUI)" 配置另做。

## 推进顺序

lab1 → lab2（心脏）→ lab3 → lab4 → lab5 → lab6。
每个脚本末尾有对照分册的自检题，做完实验再回答一遍。
