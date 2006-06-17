#	$NetBSD: Makefile.inc,v 1.12 2006/06/17 18:04:23 uwe Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

.if ${MKSOFTFLOAT} != "no"
CPPFLAGS+= -DSOFTLOFLOAT_NEED_FIXUNS -DSOFTFLOAT -DSOFTFLOATM68K_FOR_GCC
.  include <softfloat/Makefile.inc>
.endif
