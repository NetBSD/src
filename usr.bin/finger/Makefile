#	$NetBSD: Makefile,v 1.9 2002/09/18 14:00:37 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	finger
SRCS=	finger.c lprint.c net.c sprint.c util.c utmpentry.c

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP -DINET6

.include <bsd.prog.mk>
