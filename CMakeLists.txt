cmake_minimum_required(VERSION 3.16)
project(sav2csv)

set(CMAKE_C_STANDARD 11)

ADD_DEFINITIONS(-Wall -Wextra -Wconversion -pedantic -c -g -O0 -std=c99 -lm)

add_executable(sav2csv main.c common.h spssr.c spssr.h eoopc.h)

target_link_libraries(sav2csv m)