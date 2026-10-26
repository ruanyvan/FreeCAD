// SPDX-License-Identifier: LGPL-2.1-or-later
// =============================================================================
// FreeCADLabs —— C++ 原生驱动的“非界面层”调试实验
//
// 与 FreeCADCmd 的区别：本程序不进入任何 Python 交互，对 FreeCAD 的每一步
// 驱动都是 C++ 原生调用，配合 VS Code 断点观察 Base/App 两层的核心机制。
// 断点清单与每个场景的观察要点见 debug-labs/README.md。
//
// 链接：FreeCADApp + Part（注意：链接 Part 只是为了直接使用 Part::Box 等
//       C++ 类型；类型表登记仍发生在模块初始化时，见场景 0）。
//
// 构建：pixi run build-debug（根 CMakeLists 挂了 debug-labs 子目录，只构建
//       本目标可用：cmake --build build/debug --target FreeCADLabs）
// 运行：VS Code 选 “FreeCAD Labs (C++ 原生驱动)”；命令行带 auto 参数则
//       跳过人工停顿自动跑完，供无人值守验证。
// =============================================================================

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Base/BaseClass.h>  // freecad_cast
#include <Base/Type.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <clocale>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
bool g_auto = false;  // auto 模式：跳过人工停顿

void pause(const char* hint)
{
    std::cout << "\n>>> " << hint << "\n>>> （回车继续）" << std::endl;
    if (!g_auto) {
        std::string line;
        std::getline(std::cin, line);
    }
}

// 通过名字取 Shape 属性并算体积 —— 这本身就是 FC03 第五节“统一访问”的演示：
// 这里拿不到也不需要 Part::Box*，只需要 App::DocumentObject*。
double volumeOf(App::DocumentObject* obj)
{
    auto* shapeProp = freecad_cast<Part::PropertyPartShape*>(obj->getPropertyByName("Shape"));
    GProp_GProps props;
    BRepGProp::VolumeProperties(shapeProp->getValue(), props);
    return props.Mass();
}

App::PropertyFloat* floatProp(App::DocumentObject* obj, const char* name)
{
    return freecad_cast<App::PropertyFloat*>(obj->getPropertyByName(name));
}
}  // namespace

// =============================================================================
// 场景 0 | 类型系统：字符串如何变成类（FC01 / FC05）
// 断点：src/Base/Type.cpp:139  Base::Type::createType（登记每一个类型）
//       src/Base/Type.cpp:108  Base::Type::importModule（按需加载模块）
// 注意：Part 的类型登记发生在模块初始化（initPart）里，不是 DLL 加载时，
//       所以程序刚启动时用名字是查不到 "Part::Box" 的——下面先演示这一点。
// =============================================================================
void scene0_typeSystem()
{
    std::cout << "\n============ 场景 0 | 类型系统（FC01/FC05） ============\n"
                 "断点：src/Base/Type.cpp:139  Type::createType\n";
    Base::Type t = Base::Type::fromName("Part::Box");
    std::cout << "启动即查 fromName(\"Part::Box\") -> " << t.getName()
              << "（Part 模块还没初始化，类型表里没有它！）\n";
    pause("场景 0：createInstanceByName(\"Part::Box\", true) —— 断点 Type.cpp:95/108/139");

    // 公开入口：查不到类型就按需加载模块再创建（Document::addObject 用的正是它）
    if (auto* inst = static_cast<Base::BaseClass*>(
            Base::Type::createInstanceByName("Part::Box", /*loadModule=*/true))) {
        delete inst;  // 只为触发登记，实例用完即弃
    }
    t = Base::Type::fromName("Part::Box");
    std::cout << "再查 fromName(\"Part::Box\") -> " << t.getName() << "\n"
              << "沿 getParent() 上溯继承链（对照 FC03 第三节的容器继承表）：\n";
    for (Base::Type p = t; !p.isBad(); p = p.getParent()) {
        std::cout << "  " << p.getName() << "\n";
    }
}

// =============================================================================
// 场景 1 | 对象创建的两条路径 + 统一访问（FC01 / FC04 / FC03§5）
// 断点：src/App/Document.cpp:3085  Document::addObject（字符串路径）
//       src/App/DocumentObject.cpp:1217  DocumentObject::setExpression（场景 4 用）
// =============================================================================
Part::Box* scene1_createObjects(App::Document* doc)
{
    std::cout << "\n============ 场景 1 | 创建长方体的两条路径（FC01/FC04） ============\n"
                 "断点：src/App/Document.cpp:3085  Document::addObject\n";
    pause("场景 1：路径 A —— 字符串 \"Part::Box\" 查类型表创建");

    // 路径 A：运行期字符串 → 类型表 → 创建函数（通用入口，模块可按需加载）
    auto* box = freecad_cast<Part::Box*>(doc->addObject("Part::Box", "Box"));

    // 路径 B：C++ 直接 new + 指针版 addObject（编译期已知类型，Document.h:343）
    pause("场景 1：路径 B —— new Part::Box + addObject(box, \"Box001\")");
    auto* box2 = new Part::Box();
    doc->addObject(box2, "Box001");
    doc->recompute();

    // 统一访问：框架代码不必知道 Part::Box*，按名字即可读写属性（FC03§5）
    auto* len = floatProp(box2, "Length");
    std::cout << "getPropertyByName(\"Length\")->getValue() = " << len->getValue()
              << "  （统一访问：无需 Part::Box*）\n"
              << "box  体积 = " << volumeOf(box) << "\n"
              << "box2 体积 = " << volumeOf(box2) << "\n";
    return box;
}

// =============================================================================
// 场景 2 | 属性系统主链：Length.setValue(20) 的原生路径（FC03，核心）
// 断点（按命中顺序）：
//   src/App/PropertyStandard.cpp:1020  PropertyFloat::setValue（三段式）
//   src/App/Property.cpp:299           aboutToSetValue → 修改前通知
//     src/App/Document.cpp:792         onBeforeChangeProperty
//       src/App/Transactions.cpp:388   旧值快照 Copy()
//   src/App/Property.cpp:284           hasSetValue → 修改后通知
//     src/App/DocumentObject.cpp:889   onChanged（设 Touch/Enforce 标记）
//       src/Mod/Part/App/FeaturePartBox.cpp:228  Box::onChanged（立即自算）
//         src/App/DocumentObject.cpp:166         this->execute() 虚分派
//           src/Mod/Part/App/FeaturePartBox.cpp:54  Box::execute → 74 行 MakeBox
//             末尾 Shape.setValue(...) 又是一条嵌套通知链
// =============================================================================
void scene2_propertyChain(App::Document* doc, Part::Box* box)
{
    std::cout << "\n============ 场景 2 | 属性主链：Length.setValue(20)（FC03） ============\n"
                 "断点：PropertyStandard.cpp:1020 → Property.cpp:299/284 →\n"
                 "      DocumentObject.cpp:889 → FeaturePartBox.cpp:228 → :54\n";
    pause("场景 2：原生 setValue(20.0)（区别于脚本路径的 setPyObject）");

    doc->setUndoMode(1);  // 控制台下撤销默认关闭，开启后通知链里才有事务快照
    doc->openTransaction("NativeSetLength20");
    std::cout << "改前 Length = " << box->Length.getValue()
              << "，体积 = " << volumeOf(box) << "\n";
    box->Length.setValue(20.0);  // ← 全部断点在这一行依次命中
    std::cout << "改后 Length = " << box->Length.getValue()
              << "，体积 = " << volumeOf(box) << "（期望 2000）\n";
    doc->commitTransaction();

    pause("场景 2 反例：原生 setValue 不做范围钳制（FC03§4 末尾的提醒）");
    doc->openTransaction("NativeNegative");
    box->Length.setValue(-5.0);  // 脚本路径会被钳到 0；原生路径原样写入
    std::cout << "Length.getValue() = " << box->Length.getValue()
              << "  ← 原样写入了 -5！execute 随即报 \"too small\"，\n"
              << "    体积仍 = " << volumeOf(box)
              << "（算法拒绝，旧形状保留）\n";
    doc->commitTransaction();
    doc->undo();  // 顺手撤销这步错误写入 → 回到 20
    std::cout << "undo 后 Length = " << box->Length.getValue()
              << "，体积 = " << volumeOf(box) << "\n";
}

// =============================================================================
// 场景 3 | 引用属性与依赖图（FC04 / FC03§12）
// 断点：src/App/PropertyLinks.cpp:730  PropertyLink::setValue（反向链接维护）
//       src/App/DocumentObject.cpp:435  getOutList
// =============================================================================
void scene3_dependencies(App::Document* doc, Part::Box* box)
{
    std::cout << "\n============ 场景 3 | 引用属性与依赖图（FC04/FC03§12） ============\n"
                 "断点：src/App/PropertyLinks.cpp:730  PropertyLink::setValue\n";
    pause("场景 3：先改 Length=30 并创建圆柱与 Cut");

    doc->openTransaction("PlateAndCut");
    box->Length.setValue(30.0);

    auto* cyl = doc->addObject("Part::Cylinder", "Cylinder");
    floatProp(cyl, "Radius")->setValue(2.0);
    floatProp(cyl, "Height")->setValue(20.0);

    auto* cut = doc->addObject("Part::Cut", "Cut");
    pause("场景 3：cut.Base/cut.Tool 赋值（PropertyLink::setValue 断点）");
    freecad_cast<App::PropertyLink*>(cut->getPropertyByName("Base"))->setValue(box);
    freecad_cast<App::PropertyLink*>(cut->getPropertyByName("Tool"))->setValue(cyl);
    doc->recompute();
    doc->commitTransaction();

    std::cout << "cut 的 OutList（我引用谁）: ";
    for (auto* o : cut->getOutList()) std::cout << o->getNameInDocument() << " ";
    std::cout << "\nbox 的 InList（谁引用我）: ";
    for (auto* o : box->getInList()) std::cout << o->getNameInDocument() << " ";
    std::cout << "\ncut 体积 = " << volumeOf(cut)
              << "（3000 - 31.42：圆柱默认立在原点，盒子也在原点角，\n"
              << "  只有圆柱的 1/8 落进盒内——pi*2^2*20/8。想切完整的孔，\n"
              << "  可给圆柱设置 Placement 把它挪到盒子中心）\n";
}

// =============================================================================
// 场景 4 | 对象级重算 vs 文档级重算 + 表达式（FC03§9/§13）
// 断点：src/App/DocumentObject.cpp:1217  setExpression
//       src/App/PropertyExpressionEngine.cpp:692  表达式求值
//       src/App/Document.cpp:2692 拓扑排序 / :3008 _recomputeFeature
//       src/App/DocumentObject.cpp:297 mustRecompute（注意 Enforce 标记）
//       src/Mod/Part/App/FeaturePartBox.cpp:46 mustExecute
// 注意：直接 DocumentObject::recompute() 不清除 Touch/Enforce，
//       所以 setValue 触发的“立即自算”之后，文档级重算还会再算一次。
// =============================================================================
void scene4_recompute(App::Document* doc, Part::Box* box)
{
    std::cout << "\n============ 场景 4 | 两级重算 + 表达式（FC03§9/§13） ============\n"
                 "断点：Document.cpp:2692/3008、PropertyExpressionEngine.cpp:692、\n"
                 "      DocumentObject.cpp:297（mustRecompute，Enforce 已置位）\n";
    pause("场景 4：setExpression(Width = Length / 2)");

    box->setExpression(App::ObjectIdentifier(box->Width),
                       std::shared_ptr<App::Expression>(
                           App::Expression::parse(box, "Length / 2")));
    doc->recompute();  // 表达式在 _recomputeFeature 里先于 execute 求值
    std::cout << "Width = " << box->Width.getValue()
              << "（= 30/2），体积 = " << volumeOf(box) << "（期望 4500）\n";

    pause("场景 4：Length.setValue(40) —— 立即自算用的是旧 Width！");
    doc->openTransaction("Length40");
    box->Length.setValue(40.0);  // 立即自算：表达式尚未求值，Width 仍是 15
    std::cout << "赋值瞬间体积 = " << volumeOf(box)
              << "（40*15*10 = 6000：旧 Width）\n";
    doc->recompute();  // 文档级：表达式先把 Width 更新为 20，对象再算
    std::cout << "recompute 后 Width = " << box->Width.getValue()
              << "，体积 = " << volumeOf(box) << "（期望 8000）\n";
    doc->commitTransaction();
}

// =============================================================================
// 场景 5 | 事务系统：整组撤销与重做（FC09 / FC03§15）
// 断点：src/App/DocumentPyImp 不涉及——原生路径：
//       src/App/Document.cpp:405  _checkTransaction
//       src/App/Document.cpp:157  Document::undo（183 行 → Transaction::apply）
//       src/App/Transactions.cpp:177  Transaction::apply
//       src/App/PropertyStandard.cpp:1076  PropertyFloat::Paste（旧值写回）
// =============================================================================
void scene5_transaction(App::Document* doc, Part::Box* box)
{
    std::cout << "\n============ 场景 5 | 事务：撤销/重做（FC09） ============\n"
                 "断点：Document.cpp:157/405、Transactions.cpp:177/388、\n"
                 "      PropertyStandard.cpp:1076（Paste）\n";
    pause("场景 5：openTransaction → 改长度 → commitTransaction");

    doc->openTransaction("ChangeLength");
    box->Length.setValue(30.0);
    doc->commitTransaction();
    std::cout << "Length = " << box->Length.getValue()
              << "，可撤销步数 = " << doc->getAvailableUndos() << "\n";

    pause("场景 5：doc->undo()（Paste 把快照旧值写回，再次触发通知链）");
    doc->undo();
    std::cout << "undo 后 Length = " << box->Length.getValue()
              << "，体积 = " << volumeOf(box)
              << "，可重做步数 = " << doc->getAvailableRedos() << "\n";

    doc->redo();
    doc->recompute();  // 文档级重算让表达式收敛：Width 回到 Length/2
    std::cout << "redo 后 Length = " << box->Length.getValue()
              << "，Width = " << box->Width.getValue()
              << "，体积 = " << volumeOf(box) << "（期望 30*15*10 = 4500）\n";
}

// =============================================================================
// 场景 6 | 持久化：FCStd 的写出与读回（FC02 / FC03§14）
// 断点（保存）：src/App/Document.cpp:1763 saveToFile → :984 Save
//               → src/App/PropertyStandard.cpp:1056 PropertyFloat::Save
//               → src/Mod/Part/App/PropertyTopoShape.cpp:704 SaveDocFile（.brp）
// 断点（打开）：src/App/Document.cpp:1938 restore
//               → src/App/PropertyStandard.cpp:1061 Restore（内部调 setValue！）
//               → src/Mod/Part/App/FeaturePartBox.cpp:228（isRestoring 为 true）
// =============================================================================
void scene6_persistence(const std::string& path)
{
    std::cout << "\n============ 场景 6 | 持久化（FC02） ============\n"
                 "断点：保存 Document.cpp:1763/984、PropertyStandard.cpp:1056、\n"
                 "      PropertyTopoShape.cpp:704；打开 Document.cpp:1938、\n"
                 "      PropertyStandard.cpp:1061、FeaturePartBox.cpp:228\n";
    auto* doc = App::GetApplication().getActiveDocument();
    pause("场景 6：saveAs —— FCStd 本质是个 zip（Document.xml + *.brp）");
    doc->saveAs(path.c_str());
    std::cout << "已保存到 " << path << "\n";

    pause("场景 6：关闭后重新打开（留意 isRestoring 分支拦住“假编辑”）");
    std::string docName = doc->getName();
    App::GetApplication().closeDocument(docName.c_str());
    auto* doc2 = App::GetApplication().openDocument(path.c_str());
    auto* rb = freecad_cast<Part::Box*>(doc2->getObject("Box"));
    std::cout << "读回 Length = " << rb->Length.getValue()
              << "，体积 = " << volumeOf(rb) << "\n";
}

// =============================================================================
int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);  // 集成终端按 UTF-8 显示中文提示
#endif
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "auto") {
            g_auto = true;
        }
    }

    // 小数点与 FreeCADCmd 保持一致（照抄 src/Main/MainCmd.cpp 的初始化骨架）
    setlocale(LC_ALL, "");
#ifdef _WIN32
    // 窄字符串按 UTF-8 解读，与编译字符集和控制台编码保持一致。
    setlocale(LC_CTYPE, ".UTF8");
#endif
    setlocale(LC_NUMERIC, "C");

    App::Application::Config()["ExeName"] = "FreeCAD";
    App::Application::Config()["ExeVendor"] = "FreeCAD";
    App::Application::Config()["AppDataSkipVendor"] = "true";
    App::Application::Config()["RunMode"] = "Exit";
    App::Application::Config()["LoggingConsole"] = "1";

    App::Application::init(argc, argv);

    scene0_typeSystem();
    auto* doc = App::GetApplication().newDocument("Labs");
    auto* box = scene1_createObjects(doc);
    scene2_propertyChain(doc, box);
    scene3_dependencies(doc, box);
    scene4_recompute(doc, box);
    scene5_transaction(doc, box);
    scene6_persistence("debug-labs/FreeCADLabs_output.FCStd");

    App::GetApplication().closeAllDocuments();
    App::Application::destruct();
    std::cout << "\nFreeCADLabs 全部场景结束。\n";
    return 0;
}
