cmake_minimum_required(VERSION 3.10.2)
project(Multithreading_Bank_App C)

set(CMAKE_C_STANDARD 99)

add_executable(Multithreading_Bank_App main.c)

TARGET_LINK_LIBRARIES( Multithreading_Bank_App pthread)