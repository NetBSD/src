#	$NetBSD: Makefile.inc,v 1.1.2.2 2007/04/26 04:04:39 thorpej Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add.S

SRCS+=	atomic_and.S

SRCS+=	atomic_cas.S

SRCS+=	atomic_dec.S

SRCS+=	atomic_inc.S

SRCS+=	atomic_or.S

SRCS+=	atomic_swap.S

SRCS+=	membar_ops.S

.endif
