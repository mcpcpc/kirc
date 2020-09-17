cmake_minimum_required(VERSION 3.10)

project(kirc LANGUAGES C VERSION 0.0.1)

add_executable(kirc kirc.c)

target_compile_features(kirc PRIVATE c_std_99)

target_compile_options(kirc PRIVATE -g -Wall -Wextra -pedantic -Wold-style-declaration)

