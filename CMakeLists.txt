# Vesna — 跨平台构建（CMake）
# Windows:  g++/MSVC 均可；需要 advapi32（注册表）
# Linux/macOS: g++/clang 直接构建（注册表类内置在非 Windows 上返回"不支持"）
cmake_minimum_required(VERSION 3.16)
project(vesna CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(vesna
  src/cpp/vesna.cpp
  src/cpp/main.cpp
)

target_include_directories(vesna PRIVATE src/cpp)

if(WIN32)
  target_link_libraries(vesna PRIVATE advapi32)
  # 静态链接运行时（可选）
  # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:>" )
endif()

# 静态编译（MinGW 可选 -static）
if(MINGW)
  target_link_options(vesna PRIVATE -static -flto)
  target_compile_options(vesna PRIVATE -O3 -Wall -Wextra)
endif()

install(TARGETS vesna RUNTIME DESTINATION bin)
