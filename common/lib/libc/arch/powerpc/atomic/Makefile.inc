#	$NetBSD: Makefile.inc,v 1.7 2026/01/09 22:54:25 jmcneill Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread" \
	|| ${LIB} == "rump" || ${LIB} == "powerpc_espresso")

SRCS+=	atomic_add.S atomic_and.S atomic_cas.S atomic_dec.S \
	atomic_inc.S atomic_or.S atomic_swap.S
.if ${LIB} != "powerpc_espresso"
SRCS+=	membar_ops.S
.endif

.endif

.if defined(LIB) && (${LIB} == "c" || ${LIB} == "pthread")

SRCS+=	atomic_init_cas.c

.endif
