cmake_minimum_required(VERSION 3.20)

project(nsio LANGUAGES C CXX ASM)

include (cmake/arch.cmake)
include (cmake/target.cmake)
include (cmake/tools.cmake)
include (cmake/ccache.cmake)
include (cmake/clang_tidy.cmake)
include (cmake/git.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # Write compile_commands.json
set(CMAKE_LINK_DEPENDS_NO_SHARED 1) # Do not relink all depended targets on .so
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Debug;Release;MinSizeRel" CACHE STRING "" FORCE)
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Generate debug library name with a postfix.")    # To be consistent with CMakeLists from contrib libs.

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_C_STANDARD 11)
set (CMAKE_C_EXTENSIONS ON) # required by most contribs written in C
set (CMAKE_C_STANDARD_REQUIRED ON)


#add_library(global-libs INTERFACE)
set (CMAKE_COLOR_MAKEFILE ${ENABLE_COLORED_BUILD}) # works only for the makefile generator


set(COMPILER_FLAGS "${COMPILER_FLAGS} -falign-functions=32")


include (cmake/check_flags.cmake)
include (cmake/add_warning.cmake)
include(cmake/cpu_features.cmake)
#include(cmake/linux/default_libs.cmake)



if (ARCH_AMD64)
    # align branches within a 32-Byte boundary to avoid the potential performance loss when code layout change,
    # which makes benchmark results more stable.
    set(BRANCHES_WITHIN_32B_BOUNDARIES "-mbranches-within-32B-boundaries")
    if (COMPILER_GCC)
        # gcc is in assembler, need to add "-Wa," prefix
        set(BRANCHES_WITHIN_32B_BOUNDARIES "-Wa,${BRANCHES_WITHIN_32B_BOUNDARIES}")
    endif()

    set(COMPILER_FLAGS "${COMPILER_FLAGS} ${BRANCHES_WITHIN_32B_BOUNDARIES}")
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

if (COMPILER_GCC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif ()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")

set (CMAKE_CXX_FLAGS                     "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 ${DEBUG_INFO_FLAGS} ${CMAKE_CXX_FLAGS_ADD}")
set (CMAKE_CXX_FLAGS_DEBUG               "${CMAKE_CXX_FLAGS_DEBUG} -O0 ${DEBUG_INFO_FLAGS} ${CMAKE_CXX_FLAGS_ADD}")

set (CMAKE_C_FLAGS                       "${CMAKE_C_FLAGS} ${COMPILER_FLAGS} ${CMAKE_C_FLAGS_ADD}")
set (CMAKE_C_FLAGS_RELWITHDEBINFO        "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O3 ${DEBUG_INFO_FLAGS} ${CMAKE_C_FLAGS_ADD}")
set (CMAKE_C_FLAGS_DEBUG                 "${CMAKE_C_FLAGS_DEBUG} -O0 ${DEBUG_INFO_FLAGS} ${CMAKE_C_FLAGS_ADD}")

set (CMAKE_ASM_FLAGS                     "${CMAKE_ASM_FLAGS} ${COMPILER_FLAGS} ${CMAKE_ASM_FLAGS_ADD}")
set (CMAKE_ASM_FLAGS_RELWITHDEBINFO      "${CMAKE_ASM_FLAGS_RELWITHDEBINFO} -O3 ${DEBUG_INFO_FLAGS} ${CMAKE_ASM_FLAGS_ADD}")
set (CMAKE_ASM_FLAGS_DEBUG               "${CMAKE_ASM_FLAGS_DEBUG} -O0 ${DEBUG_INFO_FLAGS} ${CMAKE_ASM_FLAGS_ADD}")


set (CMAKE_POSTFIX_VARIABLE "CMAKE_${CMAKE_BUILD_TYPE_UC}_POSTFIX")

set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-pie")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fno-pie")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie -Wl,-no-pie")

include (cmake/warnings.cmake)
include (cmake/print_flags.cmake)

add_subdirectory(common)
add_subdirectory(coio)
add_subdirectory(test)


