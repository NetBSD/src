#	$NetBSD: Makefile,v 1.3 2004/11/19 21:43:40 christos Exp $

PROG= lastlogin
MAN= lastlogin.8
CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
