cmake_minimum_required(VERSION 3.0.0)


set(CMAKE_VERBOSE_MAKEFILE on)   # 开启输出编译详细过程的提示
set(TOOL_PREFIX  "x86_64-elf-")

# C编译器与参数配置
set(CMAKE_C_COMPILER "${TOOL_PREFIX}gcc")
set(CMAKE_C_FLAGS "-g -c -O0 -m32 -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables")

# 汇编器与参数配置
set(CMAKE_ASM_COMPILER "${TOOL_PREFIX}gcc")
set(CMAKE_ASM_FLAGS "-m32 -g")
set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS "asm")

# 链接器工具
set(LINKER_TOOL "${TOOL_PREFIX}ld")

# 其它工具
set(OBJCOPY_TOOL "${TOOL_PREFIX}objcopy")
set(OBJDUMP_TOOL "${TOOL_PREFIX}objdump")
set(READELF_TOOL "${TOOL_PREFIX}readelf")

# 工程，启用C语言和汇编语言
project(os LANGUAGES C)  
enable_language(ASM)

# 头文件搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/kernel/include
    ${PROJECT_SOURCE_DIR}/loader
)

# 底层的若干子项目：含内核及应用程序
add_subdirectory(./src/boot)
add_subdirectory(./src/loader)
add_subdirectory(./src/kernel)
add_subdirectory(./src/applib)

add_subdirectory(./src/shell)

