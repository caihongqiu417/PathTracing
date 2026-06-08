set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE source CMAKE_CONFIGURE_DEPENDS src/*.cpp)

add_executable(PathTracing ${source})

target_include_directories(PathTracing PUBLIC include)


target_link_libraries(PathTracing PRIVATE glm::glm)

if(MSVC)
	target_compile_options(PathTracing PRIVATE /utf-8)
endif()