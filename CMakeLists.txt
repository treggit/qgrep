cmake_minimum_required(VERSION 3.9)
project(qgrep)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
#set(CMAKE_PREFIX_PATH /usr/local/Cellar/qt/5.10.1)

find_package(Qt5Widgets REQUIRED)
add_executable(qgrep
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui worker_t.cpp worker_t.h searcher_t.cpp searcher_t.h trigram.cpp trigram.h QString_std_hash.h)

target_link_libraries(qgrep Qt5::Widgets -lpthread)
