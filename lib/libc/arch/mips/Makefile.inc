#	$NetBSD: Makefile.inc,v 1.9.2.1 2011/02/08 16:18:56 bouyer Exp $

.include <bsd.own.mk>

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
CPPFLAGS+= -DSOFTFLOAT
.endif
