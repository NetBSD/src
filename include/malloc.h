/*	$NetBSD: malloc.h,v 1.4 2019/03/09 03:52:10 christos Exp $	*/

#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdlib.h>

__BEGIN_DECLS

void *mallocx(size_t, int);
void *rallocx(void *, size_t, int);
size_t xallocx(void *, size_t, size_t, int);
size_t sallocx(void *, int);
void dallocx(void *, int);
void sdallocx(void *, size_t, int);
size_t nallocx(size_t, int);

int mallctl(const char *, void *, size_t *, void *, size_t);
int mallctltomib(const char *, size_t *, size_t *);
int mallctlbymib(const size_t *, size_t, void *, size_t *, void *, size_t);

void malloc_stats_print(void (*)(void *, const char *), void *, const char *);

size_t malloc_usable_size(const void *);

void (*malloc_message)(void *, const char *);

const char *malloc_conf;

__END_DECLS

#endif /* _MALLOC_H_ */
