#	$NetBSD: Makefile.inc,v 1.19 2025/09/10 23:07:38 nat Exp $

SRCS+=	__sigtramp2.S __m68k_read_tp.S __mmap.S

ASM+=	_lwp_getprivate.S mremap.S

CPPFLAGS+= -I.		# for assym.h

.if ${MKSOFTFLOAT} != "no"
.  if ${MKLIBCSOFTFLOAT} != "no"
.  include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT
.    if ${SOFTFLOAT_BITS} == "32"
CPPFLAGS+= -DSOFTFLOATBITS32
.    endif
.  else
#CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
CPPFLAGS+= -DSOFTFLOAT -DSOFTFLOATM68K_FOR_GCC
.include <softfloat/Makefile.fenv.inc>
.  endif
.elif ${MACHINE_ARCH} != "m68000"
.PATH : ${ARCHDIR}/hardfloat
.  include "${ARCHDIR}/hardfloat/Makefile.inc"
.endif
