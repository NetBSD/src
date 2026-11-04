#	$NetBSD: Makefile.inc,v 1.6.70.1 2026/11/04 22:36:27 perseant Exp $

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
