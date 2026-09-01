add_compile_options(-ggdb3 -Og)

set(SOURCES
    Device.cpp
    Image.cpp
    Shader.cpp

    Buffer.cpp

    ../LibGPU/Image.cpp
)

serenity_lib(LibV3DGPU v3dgpu)
target_link_libraries(LibV3DGPU PRIVATE LibCore)
