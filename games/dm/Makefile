#	$NetBSD: Makefile,v 1.10 2002/09/18 02:51:46 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

# -DLOG		log games
PROG=	dm
SRCS=	dm.c utmpentry.c
MAN=	dm.8 dm.conf.5
# shouldn't be necessary; just in case.
BINGRP= games
BINMODE=2555

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
