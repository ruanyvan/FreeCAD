# -*- coding: utf-8 -*-
# ============================================================
# 实验 4 | 文档重算调度：拓扑排序、mustRecompute、表达式引擎
# 对应分册：FC03-属性系统（第九节 文档重算、第十三节 表达式）/ FC04-对象模型
#
# 断点清单：
#   src/App/DocumentObjectPyImp.cpp:362      setExpression     绑定表达式
#   src/App/PropertyExpressionEngine.cpp:692 ExpressionEngine::execute
#   src/App/Document.cpp:2692                依赖拓扑排序处(topoSortedObjects)
#   src/App/Document.cpp:3008                _recomputeFeature
#                                            三段式: 表达式→execute→输出表达式
#   src/App/DocumentObject.cpp:297           mustRecompute
#   src/Mod/Part/App/FeaturePartBox.cpp:46   Box::mustExecute
#
# 关键观察（本实验的戏剧性所在）：
#   [2] 改 Length 后 box 已在 Box::onChanged 里"立即自算"过一次；
#       紧接着的普通 doc.recompute() 里 mustRecompute() 为 false → box 被跳过。
#       而且立即自算发生在表达式求值之前 → 用的是旧 Width！
#       （DocumentObject::recompute 直接调 execute，不跑 ExpressionEngine，
#         表达式只在 _recomputeFeature 里求值——两层重算不是一回事）
#   [3] 强制重算 doc.recompute(None, True) 才会再次进入 Box::execute
# ============================================================
import FreeCAD as App

doc = App.newDocument("Lab4")

box = doc.addObject("Part::Box", "Box")
cyl = doc.addObject("Part::Cylinder", "Cylinder")
cut = doc.addObject("Part::Cut", "Cut")
cut.Base, cut.Tool = box, cyl
doc.recompute()

input("\n[1] 按回车绑定表达式 Width = Length / 2（断点 362）")
box.setExpression("Width", "Length / 2")
doc.recompute()

input("\n[2] 按回车改 Length=40（先别 recompute，看体积用的是旧宽度）")
box.Length = 40
print("仅赋值后  Volume =", box.Shape.Volume, "(40*10*10=4000：立即自算用的是旧 Width)")
doc.recompute()
print("重算之后  Width =", box.Width, " Volume =", box.Shape.Volume,
      "(表达式已把 Width 改成 20 → 8000)")

input("\n[3] 按回车强制重算 recompute(None, True)（box 再次进入 execute）")
doc.recompute(None, True)

# 自检（对照 FC03 第九/十三节）：
#   Q1: 为什么"对象自己算过"不等于"模型整体已更新"？
#   Q2: 表达式求值发生在对象 execute 之前还是之后？在哪一层？
#   Q3: cut 依赖 box：断点 2692 处 topoSortedObjects 的顺序说明了什么？
