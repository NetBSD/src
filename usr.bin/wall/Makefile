#	$NetBSD: Makefile,v 1.12 2019/10/13 07:28:17 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
WARNS=6

USE_FORT?= yes	# setuid
PROG=	wall
SRCS=	wall.c utmpentry.c term_chk.c
BINGRP=	tty
BINMODE=2555
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who ${NETBSDSRCDIR}/usr.bin/write
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/write

COPTS.wall.c+=		${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
