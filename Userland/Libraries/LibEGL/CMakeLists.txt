set(SOURCES
    EGL.cpp
)

serenity_lib(LibEGL egl)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

    target_link_libraries(LibEGL PRIVATE PkgConfig::WAYLAND_CLIENT PkgConfig::X11 LibCore LibGfx LibGL)
else()
    target_link_libraries(LibEGL PRIVATE LibCore LibGfx LibGL)
endif()
