#	$NetBSD: Makefile,v 1.11 2005/01/10 02:58:58 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

WARNS=	3
PROG=	rlogind
MAN=	rlogind.8
DPADD=	${LIBUTIL}
LDADD=	-lutil

CPPFLAGS+=	-DSUPPORT_UTMP -DSUPPORT_UTMPX

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>
