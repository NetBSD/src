#	$NetBSD: Makefile,v 1.11.34.1 2009/05/13 19:18:43 jym Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	rlogind
MAN=	rlogind.8
DPADD=	${LIBUTIL}
LDADD=	-lutil

CPPFLAGS+=	-DSUPPORT_UTMP -DSUPPORT_UTMPX

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>
