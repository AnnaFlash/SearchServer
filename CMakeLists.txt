# CMakeList.txt: проект CMake для Final_project_plus_Test; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

project (Final_project_plus_Test)
set(CMAKE_CXX_STANDARD 17)
file(GLOB SOURCES *.cpp *.h)
add_executable("${PROJECT_NAME}" "${SOURCES}")
# Добавьте источник в исполняемый файл этого проекта.

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
