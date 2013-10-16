/*	$NetBSD */

/*
 * This file is a placeholder only, to allow Lua to be compiled from
 * unchanged sources.
 */

#include <sys/param.h>

#ifndef _LUA_INCLUDE_STRING_
#define _LUA_INCLUDE_STRING_

#define strcoll strcmp

char *strncat(char *dst, const char *src, size_t n);
size_t strspn(const char *s, const char *charset);
size_t strcspn(const char *s, const char *charset);
char *strpbrk(const char *s, const char *charset);

#endif

