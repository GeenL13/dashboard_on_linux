cmake_minimum_required(VERSION 3.16)

# 项目名称。修改这里不会影响源文件，只会影响生成的目标名称和构建信息。
project(can_data_collect LANGUAGES C)

# 使用 C11 标准。
# 如果目标板上的编译器不支持 C11，可以改成 C99；当前代码使用的语法在两者中都可用。
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 让 CMake 在构建时检查常见问题，但不使用 -Werror，避免警告直接导致构建失败。
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# pthread_create、pthread_mutex_init 等接口由 Threads::Threads 提供。
find_package(Threads REQUIRED)

# =========================
# 源文件列表
# =========================
#
# 后续新增 .c 文件时，在这里添加一行即可。例如：
#     app/new_task.c
#
# 头文件通常不必写入这个列表也能正常编译；将头文件列出可以让它们显示在
# IDE 的工程树中，因此这里一并列出。
set(PROJECT_SOURCES
    core/main.c
    core/can.c
    core/main.h
    core/can.h
    core/shared.h
    app/can_receive.c
    app/can_receive.h
    app/connect_to_display.c
    app/connect_to_display.h
    app/write_sdcard.c
    app/write_sdcard.h

)

# 生成最终可执行文件。第一个参数是可执行文件名，后面是参与编译的源文件。
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 头文件搜索路径。
# 使用 PRIVATE 表示这些路径只对当前可执行文件生效。
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/app

)

target_compile_definitions(can_data_collect PRIVATE _DEFAULT_SOURCE)

# 链接 POSIX 线程库。
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Threads::Threads
)

# 安装规则（可选）。执行 "cmake --install build" 时，程序会安装到 bin 目录。
# 交叉编译或只在开发板上直接运行时，这部分可以保留，也可以删除。
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)