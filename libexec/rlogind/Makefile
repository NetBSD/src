#	$NetBSD: Makefile,v 1.12 2009/03/16 02:24:56 lukem Exp $
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
