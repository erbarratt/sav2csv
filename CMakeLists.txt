cmake_minimum_required(VERSION 3.16)
project(sav2csv)

set(CMAKE_C_STANDARD 11)

ADD_DEFINITIONS(-Wall -Wextra -Wconversion -pedantic -c -g -O3 -std=c99)

add_executable(sav2csv main.c common.h spssr.c spssr.h eoopc.h)
