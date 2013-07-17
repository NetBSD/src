#	$NetBSD: Makefile.inc,v 1.15 2013/07/17 01:41:17 matt Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S __m68k_read_tp.S __mmap.S

ASM+=	_lwp_getprivate.S mremap.S

CPPFLAGS+= -I.		# for assym.h

.if ${MKSOFTFLOAT} != "no"
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS -DSOFTFLOAT -DSOFTFLOATM68K_FOR_GCC
.  include <softfloat/Makefile.inc>
.endif
