#	$NetBSD: Makefile.inc,v 1.2 2007/11/28 17:39:06 ad Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add.S atomic_and.S atomic_cas.S atomic_dec.S \
	atomic_inc.S atomic_or.S atomic_swap.S membar_ops.S

.endif
