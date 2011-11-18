# $NetBSD: Makefile.inc,v 1.10 2011/11/18 16:10:02 joerg Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS += -DSOFTFLOAT

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
