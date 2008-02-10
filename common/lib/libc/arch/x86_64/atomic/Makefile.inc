#	$NetBSD: Makefile.inc,v 1.3 2008/02/10 18:45:41 ad Exp $

SRCS+=	atomic.S

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif
