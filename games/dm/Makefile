#	$NetBSD: Makefile,v 1.9 2002/08/02 03:06:24 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

# -DLOG		log games
PROG=	dm
SRCS=	dm.c utmpentry.c
MAN=	dm.8 dm.conf.5
# shouldn't be necessary; just in case.
BINGRP= games
BINMODE=2555

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
