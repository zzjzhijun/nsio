# See linux/toolchain-x86_64.cmake for details about multiple load of toolchain file.
include_guard(GLOBAL)

set (CMAKE_SYSTEM_NAME "Darwin")
set (CMAKE_SYSTEM_PROCESSOR "aarch64")
set (CMAKE_C_COMPILER_TARGET "aarch64-apple-darwin")
set (CMAKE_CXX_COMPILER_TARGET "aarch64-apple-darwin")
set (CMAKE_ASM_COMPILER_TARGET "aarch64-apple-darwin")
set (CMAKE_OSX_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/../toolchain/darwin-aarch64")

set (CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)  # disable linkage check - it doesn't work in CMake

set (HAS_PRE_1970_EXITCODE "0" CACHE STRING "Result from TRY_RUN" FORCE)
set (HAS_PRE_1970_EXITCODE__TRYRUN_OUTPUT "" CACHE STRING "Output from TRY_RUN" FORCE)

set (HAS_POST_2038_EXITCODE "0" CACHE STRING "Result from TRY_RUN" FORCE)
set (HAS_POST_2038_EXITCODE__TRYRUN_OUTPUT "" CACHE STRING "Output from TRY_RUN" FORCE)
