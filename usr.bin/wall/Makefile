#	$NetBSD: Makefile,v 1.9 2003/04/21 00:05:30 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	wall
SRCS=	wall.c utmpentry.c term_chk.c
BINGRP=	tty
BINMODE=2555
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who ${NETBSDSRCDIR}/usr.bin/write
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/write

.include <bsd.prog.mk>
