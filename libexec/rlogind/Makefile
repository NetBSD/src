#	$NetBSD: Makefile,v 1.10 2004/11/15 20:45:52 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS=	3
PROG=	rlogind
MAN=	rlogind.8
DPADD=	${LIBUTIL}
LDADD=	-lutil

CPPFLAGS+=	-DINET6 -DSUPPORT_UTMP -DSUPPORT_UTMPX

.include <bsd.prog.mk>
