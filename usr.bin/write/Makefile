#	$NetBSD: Makefile,v 1.5 2002/08/02 01:59:43 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	write
SRCS=	write.c utmpentry.c
BINMODE=2555
BINGRP=	tty

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
