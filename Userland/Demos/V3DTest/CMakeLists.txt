serenity_component(
    V3DTest
    TARGETS V3DTest
)

set(SOURCES
    Triangle.cpp
    Buffer.cpp
    main.cpp
)

serenity_app(V3DTest ICON filetype-executable)
target_link_libraries(V3DTest PRIVATE LibGUI LibGfx LibCore LibMain)
