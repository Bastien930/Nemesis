cmake_minimum_required(VERSION 3.31)
project(DA C)

set(CMAKE_C_STANDARD 23)

# Build type par défaut en Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

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
        ../src/mangling.c
        ../src/Hash_Engine.c
        ../Include/Hash_Engine.h
        ../Include/Mangling.h
)

target_link_libraries(DA crypto) #pour open/ssl
target_link_libraries(DA crypt) #temporaire

# Optimisations
# Debug : pas d'optimisations agressives, ajouter -g et garder frame pointers
#set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -Wall -Wextra")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

# Release : tu peux garder ton tuning, séparé du debug
set(CMAKE_C_FLAGS_RELEASE "-O3 -fno-omit-frame-pointer -march=native -funroll-loops")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")



message(STATUS "Build: ${CMAKE_BUILD_TYPE} | Flags: ${CMAKE_C_FLAGS_RELEASE}")