set(SOURCES
    init.cpp

    DebugOutput.cpp
    DeviceTree.cpp
    GOP.cpp
    Panic.cpp
    Relocation.cpp
    Runtime.cpp
    kmalloc.cpp

    Arch/${SERENITY_ARCH}/Boot.cpp
    Arch/${SERENITY_ARCH}/MMU.cpp

    ../Library/MiniStdLib.cpp
    ../Prekernel/UBSanitizer.cpp

    ../../Userland/Libraries/LibELF/Image.cpp
    ../../Userland/Libraries/LibELF/Validation.cpp

    ../../AK/Format.cpp
    ../../AK/StringBuilder.cpp
    ../../AK/StringUtils.cpp
    ../../AK/StringView.cpp
)

add_library(KernelObject OBJECT IMPORTED)

set_property(TARGET KernelObject PROPERTY
    IMPORTED_OBJECTS ${CMAKE_CURRENT_BINARY_DIR}/../Kernel.o
)

add_compile_definitions(PREKERNEL)

add_executable(EFIPrekernel ${SOURCES} $<TARGET_OBJECTS:KernelObject>)

add_dependencies(EFIPrekernel Kernel)
add_dependencies(EFIPrekernel install_libc_headers)

target_compile_options(EFIPrekernel PRIVATE -fno-threadsafe-statics)
target_link_options(EFIPrekernel PRIVATE LINKER:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld LINKER:--no-dynamic-linker -nostdlib)
set_target_properties(EFIPrekernel PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(EFIPrekernel PRIVATE gcc)

    # Prevent naively implemented string functions (like strlen) from being "optimized" into a call to themselves.
    set_source_files_properties(../Library/MiniStdLib.cpp
        PROPERTIES COMPILE_FLAGS "-fno-tree-loop-distribution -fno-tree-loop-distribute-patterns")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    target_link_libraries(EFIPrekernel PRIVATE clang_rt.builtins)
endif()

# The PrekernelPEImageGenerator doesn't support RELR relocations.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    target_link_options(EFIPrekernel PRIVATE LINKER:-z,nopack-relative-relocs LINKER:--pack-dyn-relocs=none)
elseif("${SERENITY_ARCH}" STREQUAL "x86_64")
    target_link_options(EFIPrekernel PRIVATE LINKER:-z,nopack-relative-relocs)
endif()

target_link_options(EFIPrekernel PRIVATE LINKER:--no-warn-mismatch)

add_custom_command(
    TARGET EFIPrekernel POST_BUILD
    COMMAND $<TARGET_FILE:Lagom::PrekernelPEImageGenerator> ${CMAKE_CURRENT_BINARY_DIR}/EFIPrekernel ${CMAKE_CURRENT_BINARY_DIR}/Kernel.efi
    VERBATIM
    DEPENDS Lagom::PrekernelPEImageGenerator
    MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/EFIPrekernel
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Kernel.efi
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Kernel.efi DESTINATION boot)

# Remove options which the Prekernel environment doesn't support.
get_target_property(EFI_PREKERNEL_COMPILE_OPTIONS EFIPrekernel COMPILE_OPTIONS)
list(REMOVE_ITEM EFI_PREKERNEL_COMPILE_OPTIONS "-fsanitize-coverage=trace-pc")
list(REMOVE_ITEM EFI_PREKERNEL_COMPILE_OPTIONS "-fsanitize=kernel-address")
# list(REMOVE_ITEM EFI_PREKERNEL_COMPILE_OPTIONS "--pack-dyn-relocs=relr")
# list(REMOVE_ITEM EFI_PREKERNEL_COMPILE_OPTIONS "LINKER:--pack-dyn-relocs=relr")
set_target_properties(EFIPrekernel PROPERTIES COMPILE_OPTIONS "${EFI_PREKERNEL_COMPILE_OPTIONS}")

# get_target_property(EFI_PREKERNEL_LINK_OPTIONS EFIPrekernel LINK_OPTIONS)
# list(REMOVE_ITEM EFI_PREKERNEL_LINK_OPTIONS "--pack-dyn-relocs=relr")
# list(REMOVE_ITEM EFI_PREKERNEL_LINK_OPTIONS "LINKER:--pack-dyn-relocs=relr")
# set_target_properties(EFIPrekernel PROPERTIES LINK_OPTIONS "${EFI_PREKERNEL_LINK_OPTIONS}")
