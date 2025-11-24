cmake_minimum_required(VERSION 3.16)
project(vmsbackup_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try Qt 6 first, then fall back to Qt 5.
find_package(Qt6 COMPONENTS Widgets REQUIRED QUIET)
if(NOT Qt6_FOUND)
  find_package(Qt5 COMPONENTS Widgets REQUIRED)
  set(QT_LIB Qt5::Widgets)
else()
  set(QT_LIB Qt6::Widgets)
endif()

add_executable(vmsbackup_gui
  src/main.cpp
  src/MainWindow.cpp
  src/MainWindow.h
  src/ControlsPane.cpp
  src/ControlsPane.h
  src/BackendRunner.cpp
  src/BackendRunner.h
)

target_link_libraries(vmsbackup_gui PRIVATE ${QT_LIB})

target_include_directories(vmsbackup_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
  target_compile_options(vmsbackup_gui PRIVATE /W4)
else()
  target_compile_options(vmsbackup_gui PRIVATE -Wall -Wextra -pedantic)
endif()
