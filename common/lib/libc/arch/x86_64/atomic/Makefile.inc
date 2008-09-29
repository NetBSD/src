#	$NetBSD: Makefile.inc,v 1.5 2008/09/29 08:43:01 ad Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread")

SRCS+=	atomic.S

.endif

.if defined(LIB) && (${LIB} == "c" || ${LIB} == "pthread")

SRCS+=	atomic_init_cas.c

.endif
