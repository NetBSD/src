# $NetBSD: Makefile.inc,v 1.10.4.1 2012/11/28 02:21:41 matt Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS += -I.
CPPFLAGS += -DSOFTFLOAT -I.

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
