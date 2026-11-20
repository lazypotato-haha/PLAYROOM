
cmake_minimum_required(VERSION 3.13)

project(PLAYROOM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(game src/game.cpp)

set_target_properties(game PROPERTIES
    SUFFIX ".js"
)

target_link_options(game PRIVATE
    "--bind"
    "--no-entry"
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/words.txt@/words.txt"
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/words_alpha.txt@/words_alpha.txt"
)
