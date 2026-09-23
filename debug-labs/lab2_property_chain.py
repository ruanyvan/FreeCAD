# -*- coding: utf-8 -*-
# ============================================================
# 实验 2 | 属性系统主链：box.Length = 20 之后到底发生了什么（最核心）
# 对应分册：FC03-属性系统（第六~八节、第十八节总图）
#
# 断点清单（按命中顺序，全部先设好；★ 建议条件断点）：
#   1. src/App/PropertyContainerPyImp.cpp:691  setCustomAttributes
#        ★ 条件: !strcmp(attr,"Length")   （条件不生效就去掉，多按几次继续）
#   2. src/App/PropertyUnits.cpp:182      setPyObject  单位解析/范围钳制
#   3. src/App/PropertyStandard.cpp:1020  PropertyFloat::setValue
#        三段式结构: aboutToSetValue / 写值 / hasSetValue
#   4. src/App/Property.cpp:299   aboutToSetValue  (修改前通知)
#        5. src/App/Document.cpp:792  onBeforeChangeProperty
#        6. src/App/Transactions.cpp:388  TransactionObject::setProperty
#             旧值 10 在这里被 Copy() 存档（实验 5 撤销的伏笔）
#   7. src/App/Property.cpp:284   hasSetValue  (修改后通知)
#        8. src/App/DocumentObject.cpp:889  DocumentObject::onChanged
#             设置对象级 Touch / Enforce 标记
#        9. src/Mod/Part/App/FeaturePartBox.cpp:228  Box::onChanged
#             ★ 条件: prop == &this->Length
#       10. src/App/DocumentObject.cpp:166  this->execute()  虚函数分派
#       11. src/Mod/Part/App/FeaturePartBox.cpp:54  Box::execute
#             第 74 行 BRepPrimAPI_MakeBox；末尾 Shape.setValue(...)
#             又触发一条完整的嵌套通知链（对照第十八节总图右下角）
#
# 观察要点：
#   * 断点 6：data.property 刚被 Copy 出来，_dValue == 10（旧值快照）
#   * 断点 9：isRestoring() == false，所以立即 recompute()
#   * 断点 11 之后：调用栈窗口出现两层 hasSetValue 嵌套
# ============================================================
import FreeCAD as App

doc = App.newDocument("Lab2")
box = doc.addObject("Part::Box", "Box")
doc.recompute()
print("改前  Length =", box.Length, " Volume =", box.Shape.Volume)

input("\n[1] 断点全部设好？按回车执行 box.Length = 20（主链开跑）")
box.Length = 20
print("改后  Length =", box.Length, " Volume =", box.Shape.Volume, "(期望 2000)")

input("\n[2] 按回车执行 box.Length = \"2 cm\"（断点 2 看单位解析）")
box.Length = "2 cm"
print("长度 =", box.Length, "(仍是 20：量纲相同、仅显示单位不同)")

input("\n[3] 按回车执行 box.Length = -5（断点 2 看范围钳制）")
box.Length = -5
print("长度 =", box.Length, "(被下限 0 钳住；随后 execute 报"
      "\"Length of box too small\"，旧形状未变)")

# 自检（对照 FC03）：
#   Q1: 为什么长度设成同一个值两次，也照样走一遍通知链？
#   Q2: 旧值 10 是在哪一行、被谁保存下来的？
#   Q3: Box::onChanged 里为什么要检查 isRestoring()？
