#ifndef STDDEF_DEFINES_H
#define STDDEF_DEFINES_H

#define NULL ((void*)0)
#define size_t unsigned long long
#define ptrdiff_t long long
#define wchar_t unsigned short

#define offsetof(type, member) __builtin_offsetof(type, member)

#define max_align_t long double

#endif