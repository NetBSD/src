#	$NetBSD: Makefile,v 1.6 2002/08/02 00:10:40 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	finger
SRCS=	finger.c lprint.c net.c sprint.c util.c utmpentry.c

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP -DINET6

.include <bsd.prog.mk>
