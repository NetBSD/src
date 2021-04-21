# $NetBSD: Makefile.inc,v 1.2 2021/04/21 07:31:37 skrll Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread" \
	|| ${LIB} == "rump")

.for op in add and cas nand or sub swap xor
.for sz in 8 16 32 64
SRCS.atomic+=	atomic_${op}_${sz}.S
.endfor
.endfor
SRCS.atomic+=	atomic_dec_32.S atomic_dec_64.S
SRCS.atomic+=	atomic_inc_32.S atomic_inc_64.S
SRCS.atomic+=	membar_ops.S
#and cas nand or sub swap xor
.for op in swap
.for sz in 8 16 32 64
.for ar in acq rel acq_rel
SRCS.atomic+=	atomic_${op}_${ar}_${sz}.S
.endfor
.endfor
.endfor
#.for op in add and nand or sub xor
#SRCS.atomic+=	sync_fetch_and_${op}_8.S
#.endfor
#.for sz in 1 2 4 8
#SRCS.atomic+=	sync_bool_compare_and_swap_${sz}.S
#.endfor

.endif

SRCS.atomic+=	atomic_init_cas.c

SRCS+=	${SRCS.atomic}
