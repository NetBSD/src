#	$NetBSD: Makefile.inc,v 1.18 2021/10/31 22:24:35 thorpej Exp $

SRCS+=	__sigtramp2.S __m68k_read_tp.S __mmap.S

ASM+=	_lwp_getprivate.S mremap.S

CPPFLAGS+= -I.		# for assym.h

.if ${MKSOFTFLOAT} != "no"
#CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
CPPFLAGS+= -DSOFTFLOAT -DSOFTFLOATM68K_FOR_GCC
.  include <softfloat/Makefile.inc>
.elif ${MACHINE_ARCH} != "m68000"
.PATH : ${ARCHDIR}/hardfloat
.  include "${ARCHDIR}/hardfloat/Makefile.inc"
.endif
