set(CMAKE_CXX_STANDARD 17)

# 自动收集 src/ 下所有 .cpp；新增文件后下次 Build 会自动重新 Configure
file(GLOB source CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(PathTracing ${source})

target_include_directories(PathTracing PUBLIC include)
target_compile_definitions(PathTracing PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_FORCE_LEFT_HANDED)

target_link_libraries(PathTracing PRIVATE glm::glm)

if(MSVC)
	target_compile_options(PathTracing PRIVATE /utf-8)
endif()
