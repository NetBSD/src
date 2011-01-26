#	$NetBSD: Makefile.inc,v 1.10 2011/01/26 01:18:48 pooka Exp $

.include <bsd.own.mk>

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
CPPFLAGS+= -DSOFTFLOAT
.endif
