#	$NetBSD: Makefile,v 1.7 2002/08/02 01:52:13 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	wall
SRCS=	wall.c utmpentry.c
BINGRP=	tty
BINMODE=2555
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
