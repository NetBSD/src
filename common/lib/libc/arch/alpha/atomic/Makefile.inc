#	$NetBSD: Makefile.inc,v 1.3 2008/02/11 14:21:11 ad Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic_add.S atomic_and.S atomic_cas.S atomic_dec.S \
	atomic_inc.S atomic_or.S atomic_swap.S membar_ops.S

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif
