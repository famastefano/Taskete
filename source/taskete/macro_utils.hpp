#pragma once

#ifdef TASKETE_EXPORT_SYMBOLS
#ifdef _MSC_VER
#define TASKETE_LIB_SYMBOLS __declspec(dllexport)
#else // Clang
#define TASKETE_LIB_SYMBOLS __attribute__((visibility("default")))
#endif
#else // import symbols
#ifdef _MSC_VER
#define TASKETE_LIB_SYMBOLS __declspec(dllimport)
#else // Clang
#define TASKETE_LIB_SYMBOLS
#endif
#endif