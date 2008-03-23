#	$NetBSD: Makefile.inc,v 1.1.2.2 2008/03/23 00:12:41 matt Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c \
	atomic_add_64_cas.c atomic_add_64_nv_cas.c \
	atomic_and_32_cas.c atomic_and_32_nv_cas.c \
	atomic_and_64_cas.c atomic_and_64_nv_cas.c \
	atomic_or_32_cas.c atomic_or_32_nv_cas.c \
	atomic_or_64_cas.c atomic_or_64_nv_cas.c

SRCS+=	atomic.S

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS += atomic_init_cas.c

.endif
