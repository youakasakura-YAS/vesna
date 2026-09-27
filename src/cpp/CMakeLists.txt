cmake_minimum_required(VERSION 3.10)
project(vesna LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/O2 /utf-8 /D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-O2)
endif()

add_executable(vesna
    vesna.cpp
    lsp.cpp
    main.cpp
)

if(WIN32)
    target_link_libraries(vesna ws2_32 advapi32)
endif()

install(TARGETS vesna RUNTIME DESTINATION bin)
