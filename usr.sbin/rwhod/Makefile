#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$NetBSD: Makefile,v 1.15 2019/10/13 07:28:21 mrg Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	rwhod
SRCS=	rwhod.c utmpentry.c
MAN=	rwhod.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.rwhod.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
