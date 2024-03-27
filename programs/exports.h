#pragma once

#if defined _WIN32 || defined __CYGWIN__
#ifdef EXPORT_SYMBOLS
#ifdef __GNUC__
#define EXPORTED __attribute__((dllexport))
#else
#define EXPORTED __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define EXPORTED __attribute__((dllimport))
#else
#define EXPORTED __declspec(dllimport)
#endif
#endif
#define INTERNAL
#define attribute_deprecated __declspec(deprecated)
#else
#if __GNUC__ >= 4
#define EXPORTED __attribute__ ((visibility("default")))
#define INTERNAL  __attribute__ ((visibility("hidden")))
#define attribute_deprecated __attribute__ ((deprecated))
#else
#define EXPORTED
#define INTERNAL
#define attribute_deprecated
#endif
#endif