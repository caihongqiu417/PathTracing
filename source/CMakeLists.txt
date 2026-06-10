set(CMAKE_CXX_STANDARD 17)

# 只收集重构后的源码目录，避免把旧位置的同名 .cpp 一起编进去。
file(GLOB source_main CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
file(GLOB source_camera CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/camera/*.cpp")
file(GLOB source_shape CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/shape/*.cpp")
file(GLOB source_thread CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/thread/*.cpp")
file(GLOB source_util CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp")

set(source
	${source_main}
	${source_camera}
	${source_shape}
	${source_thread}
	${source_util}
)

add_executable(PathTracing ${source})

target_include_directories(PathTracing PUBLIC
	include
	include/camera
	include/shape
	include/thread
	include/util
)
target_compile_definitions(PathTracing PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_FORCE_LEFT_HANDED)

target_link_libraries(PathTracing PRIVATE glm::glm)

if(MSVC)
	target_compile_options(PathTracing PRIVATE /utf-8)
endif()
