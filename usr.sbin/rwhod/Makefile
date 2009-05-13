#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$NetBSD: Makefile,v 1.13.20.1 2009/05/13 19:20:38 jym Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	rwhod
SRCS=	rwhod.c utmpentry.c
MAN=	rwhod.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
