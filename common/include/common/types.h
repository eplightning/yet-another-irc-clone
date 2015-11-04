#pragma once

#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>

#include <limits.h>

#define YAIC_NAMESPACE namespace YAIC {
#define END_NAMESPACE }
#define UNUSED(x) (void)(x)

YAIC_NAMESPACE

// STL
typedef std::string String;
template<class T> using UniquePtr = std::unique_ptr < T > ;
template<class T> using SharedPtr = std::shared_ptr < T > ;
template<class T> using WeakPtr = std::weak_ptr < T > ;
template<class T, class T2> using Map = std::map < T, T2 > ;
template<class T, class T2> using HashMap = std::unordered_map < T, T2 > ;
template<class T> using Vector = std::vector < T > ;

// unsigned int
typedef unsigned int uint;

// 8-bit integer
#if UCHAR_MAX == 255
typedef unsigned char u8;
typedef signed char s8;
#else
#error "No 8-bit int type found"
#endif

// 16-bit integer
#if USHRT_MAX == 65535
typedef unsigned short u16;
typedef signed short s16;
#elif UINT_MAX == 65535
typedef unsigned int u16;
typedef signed int s16;
#else
#error "No 16-bit int type found"
#endif

// 32-bit integer
#if UINT_MAX == 4294967295
typedef unsigned int u32;
typedef signed int s32;
#elif ULONG_MAX == 4294967295
typedef unsigned long u32;
typedef signed long s32;
#elif USHRT_MAX == 4294967295
typedef unsigned short u32;
typedef signed short s32;
#else
#error "No 32-bit int type found"
#endif

// 64-bit integer
#if ULONG_MAX == 18446744073709551615ULL
typedef unsigned long u64;
typedef signed long s64;
#elif ULLONG_MAX == 18446744073709551615ULL
typedef unsigned long long u64;
typedef signed long long s64;
#elif UINT_MAX == 18446744073709551615ULL
typedef unsigned int u64;
typedef signed int s64;
#else
#error "No 64-bit int type found"
#endif

END_NAMESPACE
