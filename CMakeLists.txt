# 指定构建本项目所需的最低 CMake 版本为 3.7
# 如果当前 CMake 版本低于 3.7，会报错并停止运行
cmake_minimum_required(VERSION 3.14)

# 定义项目名称为 PathTracing
# 并声明项目使用 C++ 语言（LANGUAGES CXX）
# CXX 是 CMake 中对 C++ 的标识符
project(
	PathTracing
	LANGUAGES CXX
)

# 添加子目录 source，让 CMake 处理 source 文件夹里的 CMakeLists.txt
# 通常该文件夹存放项目的核心源代码
add_subdirectory(source)

# 添加子目录 thirdparty，让 CMake 处理 thirdparty 文件夹里的 CMakeLists.txt
# 通常该文件夹存放第三方依赖库（如 glm, stb_image, tinyobjloader 等）
add_subdirectory(thirdparty)