set(SOURCES
    init.cpp
    Arch/${SERENITY_ARCH}/boot.S
    ../../Userland/Libraries/LibELF/Relocation.cpp
    ../Library/MiniStdLib.cpp
)

add_executable(EFIPrekernel ${SOURCES})
add_dependencies(EFIPrekernel install_libc_headers)

target_compile_options(EFIPrekernel PRIVATE -static-pie -fno-threadsafe-statics -fno-sanitize=all -fno-stack-protector) # TODO: UBSAN, stack protector on x86?
target_link_options(EFIPrekernel PRIVATE LINKER:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld LINKER:--no-dynamic-linker -nostdlib)
set_target_properties(EFIPrekernel PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

if ("${SERENITY_ARCH}" STREQUAL "riscv64" OR "${SERENITY_ARCH}" STREQUAL "aarch64")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(EFIPrekernel PRIVATE -fno-stack-protector -fno-sanitize=all -ftrivial-auto-var-init=uninitialized -fno-tree-loop-distribution -fno-tree-loop-distribute-patterns)
    else()
        target_compile_options(EFIPrekernel PRIVATE -fno-stack-protector -fno-sanitize=all -ftrivial-auto-var-init=uninitialized)
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(EFIPrekernel PRIVATE gcc)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    target_link_libraries(EFIPrekernel PRIVATE clang_rt.builtins)
endif()

# PE doesn't have program headers, so explicitly list the required sections here.
# edk2 doesn't like sections with a virtual address of zero.
set(SECTIONS_TO_INCLUDE_IN_PE_IMAGE
    -j .text
    -j .rodata
    -j .data
    -j .bss
    -j .reloc
    -j .dynamic
    -j .dynsym
    -j .dynstr
    -j .rela.dyn
    -j .relr.dyn
    -j .got
)

add_custom_command(
    TARGET EFIPrekernel POST_BUILD
    # COMMAND ${CMAKE_OBJCOPY} -R .comment -R .note.gnu.property -R .riscv.attributes --image-base 0 -O efi-app-${SERENITY_ARCH} ${CMAKE_CURRENT_BINARY_DIR}/EFIPrekernel ${CMAKE_CURRENT_BINARY_DIR}/Prekernel.efi
    COMMAND ${CMAKE_OBJCOPY} ${SECTIONS_TO_INCLUDE_IN_PE_IMAGE} -O efi-app-${SERENITY_ARCH} ${CMAKE_CURRENT_BINARY_DIR}/EFIPrekernel ${CMAKE_CURRENT_BINARY_DIR}/Prekernel.efi
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Prekernel.efi
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Prekernel.efi" DESTINATION boot)

# Remove options which the Prekernel environment doesn't support.
get_target_property(EFI_PREKERNEL_TARGET_OPTIONS EFIPrekernel COMPILE_OPTIONS)
list(REMOVE_ITEM EFI_PREKERNEL_TARGET_OPTIONS "-fsanitize-coverage=trace-pc")
list(REMOVE_ITEM EFI_PREKERNEL_TARGET_OPTIONS "-fsanitize=kernel-address")
set_target_properties(EFIPrekernel PROPERTIES COMPILE_OPTIONS "${EFI_PREKERNEL_TARGET_OPTIONS}")
