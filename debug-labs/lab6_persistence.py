# -*- coding: utf-8 -*-
# ============================================================
# 实验 6 | 持久化：FCStd 文件里到底写了什么、怎么读回来
# 对应分册：FC02-持久化系统 / FC03-属性系统（第十四节）
#
# 断点清单：
#   保存链：
#     src/App/Document.cpp:1763   saveToFile        打包 zip 的总入口
#     src/App/Document.cpp:984    Document::Save    遍历对象写 ObjectData
#     src/App/PropertyStandard.cpp:1056  PropertyFloat::Save
#                                           <Float value="20"/> 出自这里
#     src/Mod/Part/App/PropertyTopoShape.cpp:364  PropertyPartShape::Save
#     src/Mod/Part/App/PropertyTopoShape.cpp:704  SaveDocFile
#                                           形状写进独立 .brp 文件
#   打开链：
#     src/App/Document.cpp:1938   restore
#     src/App/PropertyStandard.cpp:1061  PropertyFloat::Restore
#                                           注意它调用的是 setValue()！
#     src/Mod/Part/App/FeaturePartBox.cpp:228  Box::onChanged
#                                           这次 isRestoring()==true
#                                           → 不立即重算（与实验 2 的关键差异）
#
# 观察要点：
#   * FCStd 本质是 zip：Document.xml + 每个 Shape 一个 .brp 附属文件
#   * Restore 走 setValue → 触发的通知链与"用户编辑"完全相同，
#     全靠 isRestoring() 把"读文件"拦在"假编辑"之外
# ============================================================
import zipfile
import FreeCAD as App

doc = App.newDocument("Lab6")
box = doc.addObject("Part::Box", "Box")
box.Length = 20
doc.recompute()

path = "debug-labs/lab6_output.FCStd"   # cwd = 仓库根目录

input("\n[1] 按回车保存（断点 1763/984/1056/364/704）")
doc.saveAs(path)
print("包内文件:", zipfile.ZipFile(path).namelist())

xml = zipfile.ZipFile(path).read("Document.xml").decode("utf-8", "ignore")
i = xml.find('name="Length"')
print("Document.xml 中 Length 附近片段:\n", xml[i-60:i+90])

input("\n[2] 按回车重新打开（断点 1938/1061/228）")
doc2 = App.openDocument(path)
b = doc2.getObject("Box")
print("读回  Length =", b.Length, " Volume =", b.Shape.Volume)
print("留意断点 228 这次的分支：isRestoring() 为 true，没有触发立即 execute")

# 自检（对照 FC02）：
#   Q1: 数值写在 Document.xml 里，形状为什么单独放 .brp 文件？
#   Q2: 恢复期间每次 setValue 都走通知链，为什么不引发连环重算？
