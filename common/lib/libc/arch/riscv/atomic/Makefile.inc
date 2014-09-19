#	$NetBSD: Makefile.inc,v 1.1 2014/09/19 17:36:24 matt Exp $

.for op in add and cas nand or sub swap xor
sizes=32
.if "${MLIBDIR}" != ""
.if ${COMMON_MACHINE_ARCH} == "riscv64"
sizes+=64
.endif
.elif ${MACHINE_ARCH} == "riscv64"
sizes+=64
.endif
.for sz in ${sizes}
SRCS.atomic+=	atomic_${op}_${sz}.S
.endfor
.for sz in 8 16
SRCS.atomic+=	atomic_${op}_${sz}_cas.c
.endfor
.endfor
SRCS.atomic+=	membar_ops.S
SRCS.atomic+=	atomic_cas_by_cas32.c

.if defined(LIB) && (${LIB} != "kern" && ${LIB} != "rump")

SRCS.atomic+=	atomic_init_cas.c

.endif #LIB

SRCS+=	${SRCS.atomic}
