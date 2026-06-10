# ָ����������Ŀ�������� CMake �汾Ϊ 3.7
# �����ǰ CMake �汾���� 3.7���ᱨ����ֹͣ����
cmake_minimum_required(VERSION 3.14)

# ������Ŀ����Ϊ PathTracing
# ��������Ŀʹ�� C++ ���ԣ�LANGUAGES CXX��
# CXX �� CMake �ж� C++ �ı�ʶ��
project(
	PathTracing
	LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Config types" FORCE)
endif()
set(CMAKE_BUILD_TYPE Release)

# ������Ŀ¼ source���� CMake ���� source �ļ������ CMakeLists.txt
# ͨ�����ļ��д����Ŀ�ĺ���Դ����
add_subdirectory(source)

# ������Ŀ¼ thirdparty���� CMake ���� thirdparty �ļ������ CMakeLists.txt
# ͨ�����ļ��д�ŵ����������⣨�� glm, stb_image, tinyobjloader �ȣ�
add_subdirectory(thirdparty)