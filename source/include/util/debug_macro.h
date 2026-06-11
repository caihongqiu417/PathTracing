#pragma once

// 用法：DEBUG_LINE( 需要调试时才编译的代码 )
#ifdef WITH_DEBUG_INFO
    #define DEBUG_LINE(...) __VA_ARGS__  // 调试模式：保留参数中的代码
#else
    #define DEBUG_LINE(...)              // 发布模式：参数代码被忽略
#endif