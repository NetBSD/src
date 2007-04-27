#	$NetBSD: Makefile.inc,v 1.1.2.1 2007/04/27 05:57:38 thorpej Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add.c

SRCS+=	atomic_and.c

SRCS+=	atomic_cas.S

SRCS+=	atomic_dec.c

SRCS+=	atomic_inc.c

SRCS+=	atomic_or.c

SRCS+=	atomic_swap.c

SRCS+=	membar_ops.S

.endif
