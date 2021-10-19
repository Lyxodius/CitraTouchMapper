cmake_minimum_required(VERSION 3.0)
project(CitraTouchMapper)

set(CMAKE_CXX_STANDARD 23)

add_executable(CitraTouchMapper main.cpp rgb.cpp)

SET(LIBS XInput)
TARGET_LINK_LIBRARIES(CitraTouchMapper ${LIBS})