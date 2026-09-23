# -*- coding: utf-8 -*-
# ============================================================
# 实验 1 | 类型系统 + Python 绑定：字符串 "Part::Box" 如何变成 C++ 对象
# 对应分册：FC01-类型系统 / FC10-Python绑定 / FC04-对象模型(开头)
#
# 运行方式：VS Code 打开本文件 → 按下面清单设好断点 → F5 选
#          "FreeCADCmd 脚本实验 (运行当前文件)"，在集成终端里按提示操作。
#
# 断点清单（先全部设好，再按第一个回车）：
#   src/App/DocumentPyImp.cpp:314    DocumentPy::addObject       Python 方法入口
#   src/App/Document.cpp:3085        Document::addObject         应用层实现
#   src/Base/Type.cpp:95             Type::createInstanceByName  拿 "Part::Box" 查类型表
#   src/Base/Type.cpp:108            Type::importModule          按需加载 Part 模块
#
# 观察要点：
#   * 断点 3 处 typeName 就是 "Part::Box"——FC01 的核心一幕：
#     字符串 → 类型编号 → 登记记录 → 创建函数 new Part::Box
#   * addObject 返回前在监视窗口加 box / 局部变量，动态类型显示 Part::Box*
#   * 本脚本没有 import Part：模块是被 createInstanceByName 按需加载的
#   * 两个长方体 TypeId 相同（共享类型记录），但 Length 各存各的
# ============================================================
import FreeCAD as App

doc = App.newDocument("Lab1")

input("\n[1] 断点设好了？按回车创建 Part::Box ...")
box = doc.addObject("Part::Box", "Box")

print("内部名称 :", box.Name)
print("标签     :", box.Label)
print("TypeId   :", box.TypeId)
print("属性列表 :", box.PropertiesList)

doc.recompute()
print("默认体积 :", box.Shape.Volume, "(10 x 10 x 10)")

input("\n[2] 按回车创建第二个长方体（两次实例、同一份类型登记）...")
box2 = doc.addObject("Part::Box", "Box001")
box2.Length = 50
print("box.Length  =", box.Length)
print("box2.Length =", box2.Length, " <- 各自独立，类共享的只是属性说明表")

# 自检（对照 FC01）：
#   Q1: 类型记录(TypeData)里存的是"这一个长方体的尺寸"吗？
#   Q2: 为什么脚本不 import Part 也能 addObject("Part::Box")？
