project(PhysarumSimulation)

set(CMAKE_CXX_STANDARD 17)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vulkan

add_subdirectory(vendors/Cashew)

# if on windows include the vulkan sdk
if(WIN32)
    # go to C:\VulkanSDK\
    set(VULKAN_SDK "C:/VulkanSDK/1.3.268.0")
    set(ENV{VULKAN_SDK} ${VULKAN_SDK})
    include_directories($ENV{VULKAN_SDK}/Include)
endif (WIN32)


add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC vendors/Cashew/CashewLib/src/)
target_link_libraries(${PROJECT_NAME} Cashew)
