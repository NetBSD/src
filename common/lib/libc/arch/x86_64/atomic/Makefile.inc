#	$NetBSD: Makefile.inc,v 1.6.50.2 2020/04/21 19:37:48 martin Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread" \
	|| ${LIB} == "rump")

SRCS+=	atomic.S

.endif

.if defined(LIB) && (${LIB} == "c" || ${LIB} == "pthread")

SRCS+=	atomic_init_cas.c

.endif
