#	$NetBSD: Makefile,v 1.4 2002/08/01 23:51:20 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	users
SRCS=	users.c utmpentry.c

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
