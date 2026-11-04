#	$NetBSD: Makefile.inc,v 1.18.4.1 2026/11/04 23:19:12 perseant Exp $

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
