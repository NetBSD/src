#	$NetBSD: Makefile.inc,v 1.4 2008/02/11 14:21:12 ad Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic.S

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif
