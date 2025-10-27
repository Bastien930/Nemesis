cmake_minimum_required(VERSION 4.1)
project(DA C)

set(CMAKE_C_STANDARD 23)

include_directories(../Include)

add_executable(DA
        ../src/main.c
        ../src/Option.c
        ../src/Shadow.c
        ../src/brute_force.c
        ../src/Config.c
        ../Include/main.h
        ../src/Utils.c
        ../Include/Utils.h
        ../Include/Shdow_io.h
        ../src/Shadow_io.c
)
