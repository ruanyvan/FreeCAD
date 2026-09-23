# -*- coding: utf-8 -*-
# ============================================================
# 实验 5 | 事务系统：撤销为什么能整组恢复
# 对应分册：FC09-事务系统 / FC03-属性系统（第十五节）
#
# !! 前置知识：控制台模式下撤销默认是关闭的（GUI 由界面层开启，
#    DocumentP.h 里 iUndoMode{0}），第一步必须 doc.UndoMode = 1
#
# 断点清单：
#   src/App/DocumentPyImp.cpp:554   openTransaction    事务边界（撤销粒度由此决定）
#   src/App/Document.cpp:405        _checkTransaction  变更时检查/自动开事务
#   src/App/Transactions.cpp:388    TransactionObject::setProperty
#                                        旧值快照 Copy()（实验 2 断点 6 的下半场）
#   src/App/Document.cpp:157        Document::undo
#                                   第 183 行附近 apply(*this, false)
#                                   → src/App/Transactions.cpp:177 Transaction::apply
#   src/App/PropertyStandard.cpp:1076  PropertyFloat::Paste
#                                        旧值写回，且 Paste 内部再次走
#                                        aboutToSetValue/hasSetValue 通知链
#
# 观察要点：
#   * 提交后 doc.UndoCount 从 0 变 1（mUndoTransactions +1）
#   * undo 时 Paste 的 _dValue 是 10（快照旧值直接写回，不经过 20）
#   * 撤销全程没有一行界面代码参与，长度却"自己"变回去了——
#     因为通知链在数据层，界面只是这颗信号的订阅者之一
# ============================================================
import FreeCAD as App

doc = App.newDocument("Lab5")
box = doc.addObject("Part::Box", "Box")
doc.recompute()

doc.UndoMode = 1   # 控制台必须手动开启！

input("\n[1] 按回车：开事务 → 改长度 → 提交（断点 554/405/388）")
doc.openTransaction("修改尺寸")
box.Length = 30
doc.commitTransaction()
print("提交后 Length =", box.Length, " 可撤销步数 =", doc.UndoCount)

input("\n[2] 按回车：doc.undo()（断点 157/177/1076）")
doc.undo()
print("撤销后 Length =", box.Length, "(直接回到 10，不经过任何中间值)")

input("\n[3] 按回车：doc.redo()")
doc.redo()
print("重做后 Length =", box.Length, "(又到 30)")

# 自检（对照 FC09）：
#   Q1: 为什么事务必须在"新值覆盖旧值之前"就开始记录？
#   Q2: openTransaction/commitTransaction 之间改 3 个属性，
#       撤销时为什么一步全部恢复？撤销粒度由什么决定？
#   Q3: 撤销写回旧值用 Paste 而不是直接 _dValue 赋值，多换来了什么？
