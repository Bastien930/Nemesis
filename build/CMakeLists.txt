cmake_minimum_required(VERSION 3.31)
project(Nemesis C)

set(CMAKE_C_STANDARD 23)

# Build type par défaut en Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include_directories(../Include)


add_executable(Nemesis
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
        ../Include/log.h
        ../src/log.c
        ../src/hashSet.c
        ../Include/hashSet.h
        ../src/Dictionnary.c
        ../Include/Dictionnary.h
)

target_link_libraries(Nemesis crypto) #pour open/ssl
target_link_libraries(Nemesis z) #pour open/ssl
target_link_libraries(Nemesis crypt) #temporaire
find_package(OpenMP REQUIRED)

if(OpenMP_C_FOUND)
    target_link_libraries(Nemesis OpenMP::OpenMP_C)
endif()

# Optimisations
# Debug : pas d'optimisations agressives, ajouter -g et garder frame pointers
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -Wall -Wextra -fopenmp")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

# Release : tu peux garder ton tuning, séparé du debug
#set(CMAKE_C_FLAGS_RELEASE "-O3 -fno-omit-frame-pointer -march=native -funroll-loops -fopenmp")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")



message(STATUS "Build: ${CMAKE_BUILD_TYPE} | Flags: ${CMAKE_C_FLAGS_RELEASE}")


set_target_properties(Nemesis PROPERTIES
        OUTPUT_NAME nemesis
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix" FORCE)
endif()

# Installation de l'exécutable
install(TARGETS Nemesis
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

# Message après installation
install(CODE "
    message(STATUS \"======================================\")
    message(STATUS \"Nemesis installé : ${CMAKE_INSTALL_PREFIX}/bin/nemesis\")
    message(STATUS \"======================================\")
")