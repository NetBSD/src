#	$NetBSD: Makefile,v 1.9 2002/08/20 14:01:49 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	uucpd
MAN=	uucpd.8
LDADD+= -lcrypt -lutil
DPADD+=	${LIBCRYPT} ${LIBUTIL}
CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX

.include <bsd.prog.mk>
