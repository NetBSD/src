#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$NetBSD: Makefile,v 1.10 2002/08/02 02:38:15 christos Exp $

PROG=	rwhod
SRCS=	rwhod.c utmpentry.c
MAN=	rwhod.8

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
