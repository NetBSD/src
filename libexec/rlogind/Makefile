#	$NetBSD: Makefile,v 1.9 2002/08/20 13:14:41 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rlogind
MAN=	rlogind.8
DPADD=	${LIBUTIL}
LDADD=	-lutil

CPPFLAGS+=	-DINET6 -DSUPPORT_UTMP -DSUPPORT_UTMPX

.include <bsd.prog.mk>
