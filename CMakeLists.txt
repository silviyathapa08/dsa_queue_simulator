cmake_minimum_required(VERSION 3.15)
project(YourProjectName)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Optionally enforce a consistent MSVC runtime: set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add SDL2 path (make sure the path is correct)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/lib/SDL2_install")

# Find SDL2
find_package(SDL2 REQUIRED)

# --------------------------------------------
# Create MainApp executable (using main.c and traffic_simulation.c)
# --------------------------------------------
add_executable(MainApp
    main.c
    traffic_simulation.c
)

target_include_directories(MainApp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}   # Use SDL2's include directories
)

target_link_libraries(MainApp PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
)

# --------------------------------------------
# Create GeneratorApp executable (using generator.c and traffic_simulation.c)
# --------------------------------------------
add_executable(GeneratorApp
    generator.c
    traffic_simulation.c   # Added to provide createVehicle and other functions
)

target_include_directories(GeneratorApp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(GeneratorApp PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
)
