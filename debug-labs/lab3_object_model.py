# -*- coding: utf-8 -*-
# ============================================================
# 实验 3 | 对象模型与依赖图：OutList / InList 是怎么长出来的
# 对应分册：FC04-对象模型 / FC03-属性系统（第十二节 引用属性）
#
# 断点清单：
#   src/App/PropertyLinks.cpp:730    PropertyLink::setValue
#       设置 cut.Base = box 时：存指针 + 维护反向链接(InList) + aboutTo/hasSet
#   src/App/DocumentObject.cpp:435   DocumentObject::getOutList
#       框架遍历全部 PropertyLink 类属性收集依赖边
#
# 观察要点：
#   * PropertyLink::setValue 里 _removeBackLink / _addBackLink：
#     "乙引用甲 → 甲的 InList 含乙"；OutList/InList 方向别记反
#   * 依赖关系完全寄居在引用属性里——没有引用属性，两个对象就是孤岛
#   * 这些边就是实验 4 里拓扑排序要吃的原料
# ============================================================
import FreeCAD as App

doc = App.newDocument("Lab3")

box = doc.addObject("Part::Box", "Box")
box.Length = 30
cyl = doc.addObject("Part::Cylinder", "Cylinder")
cyl.Radius, cyl.Height = 2, 20
cut = doc.addObject("Part::Cut", "Cut")

input("\n[1] 断点设好？按回车执行 cut.Base = box（PropertyLink::setValue）")
cut.Base = box
cut.Tool = cyl

print("cut 的 OutList（我引用了谁）:", [o.Name for o in cut.OutList])
print("box 的 InList（谁引用了我）:", [o.Name for o in box.InList])
print("cyl 的 InList              :", [o.Name for o in cyl.InList])

doc.recompute()
print("切割后体积 =", round(cut.Shape.Volume, 1),
      "(期望 3000 - pi*4*20 ≈ 2748.7)")

# 自检（对照 FC04 / FC03 十二节）：
#   Q1: cut 的六个面是六个独立文档对象，还是 Shape 内部的子元素？
#   Q2: 如果只是给 Cut 类加一个普通 DocumentObject* 成员而不注册属性，
#       文档能知道 cut 依赖 box 吗？重算顺序还能保证吗？
