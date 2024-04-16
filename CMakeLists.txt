project(PhysarumSimulation)

set(CMAKE_CXX_STANDARD 17)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendors/Cashew)

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC vendors/Cashew/CashewLib/src/)
target_link_libraries(${PROJECT_NAME} Cashew)

