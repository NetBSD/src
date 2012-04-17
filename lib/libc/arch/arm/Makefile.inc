# $NetBSD: Makefile.inc,v 1.9.6.1 2012/04/17 00:05:12 yamt Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS += -DSOFTFLOAT

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
