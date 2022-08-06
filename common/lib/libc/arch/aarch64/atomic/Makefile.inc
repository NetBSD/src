# $NetBSD: Makefile.inc,v 1.5 2022/08/06 21:31:33 riastradh Exp $

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
.for op in swp cas clr set eor add
.for sz in 1 2 4 8
.for ar in _relax _acq _rel _acq_rel _sync
__aarch64_${op}${sz}${ar}.S: __aarch64_lse.S
	${_MKTARGET_CREATE}
	printf '#define OP ${op}\n#define OP_${op}\n#define SZ ${sz}\n#define AR ${ar}\n#define AR${ar}\n#include "__aarch64_lse.S"\n' > ${.TARGET}
SRCS.gen+=	__aarch64_${op}${sz}${ar}.S
.endfor
.endfor
.endfor
.for op in casp
.for ar in _relax _acq _rel _acq_rel _sync
__aarch64_${op}${ar}.S: __aarch64_lse.S
	${_MKTARGET_CREATE}
	printf '#define OP ${op}\n#define OP_${op}\n#define AR ${ar}\n#define AR${ar}\n#include "__aarch64_lse.S"\n' > ${.TARGET}
SRCS.gen+=	__aarch64_${op}${ar}.S
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

SRCS+=		${SRCS.atomic} ${SRCS.gen}
CLEANFILES+=	${SRCS.gen}
