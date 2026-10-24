# SPDX-License-Identifier: LGPL-2.1-or-later
# FreeCADLabs —— C++ 原生驱动的“非界面层”调试实验目标
# 详见 debug-labs/README.md；根 CMakeLists.txt 中由
# option(FREECAD_BUILD_DEBUG_LABS) 控制是否挂载本目录。

add_executable(FreeCADLabs cpp/LabsMain.cpp)

target_include_directories(FreeCADLabs PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

# 说明：链接 Part 后，可执行文件的导入表引用 Part.pyd；
# 运行时需要能按 PATH 找到它——launch.json 的调试配置已把
# build/debug/Mod/Part 加入 PATH（上游 tests/CMakeLists.txt 注明
# 模块测试在 Windows 上正是因为缺 PATH 而无法运行）。
target_link_libraries(FreeCADLabs PRIVATE
    FreeCADApp
    Part
    ${OCC_LIBRARIES}
)

set_target_properties(FreeCADLabs PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(MSVC)
    # 源码中的中文提示按 UTF-8 编译/执行
    target_compile_options(FreeCADLabs PRIVATE /utf-8)
endif()
