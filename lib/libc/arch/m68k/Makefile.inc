#	$NetBSD: Makefile.inc,v 1.13.28.2 2014/05/22 11:36:47 yamt Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S __m68k_read_tp.S __mmap.S

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
