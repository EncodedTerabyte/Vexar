#ifndef STDINT_DEFINES_H
#define STDINT_DEFINES_H

#define int8_t signed char
#define int16_t short
#define int32_t int
#define int64_t long long

#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long

#define intptr_t long long
#define uintptr_t unsigned long long

#define INT8_MIN (-128)
#define INT8_MAX (127)
#define UINT8_MAX (255)

#define INT16_MIN (-32768)
#define INT16_MAX (32767)
#define UINT16_MAX (65535)

#define INT32_MIN (-2147483648)
#define INT32_MAX (2147483647)
#define UINT32_MAX (4294967295U)

#define INT64_MIN (-9223372036854775808LL)
#define INT64_MAX (9223372036854775807LL)
#define UINT64_MAX (18446744073709551615ULL)

#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX
#define UINTPTR_MAX UINT64_MAX

#define SIZE_MAX UINT64_MAX

#define PTRDIFF_MIN INT64_MIN
#define PTRDIFF_MAX INT64_MAX

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#define WCHAR_MIN 0
#define WCHAR_MAX UINT16_MAX

#define WINT_MIN 0
#define WINT_MAX UINT16_MAX

#endif