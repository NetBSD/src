#	$NetBSD: Makefile,v 1.15 2005/02/20 05:45:55 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	rshd
MAN=	rshd.8

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

CPPFLAGS+=-DLOGIN_CAP

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
DPADD+= ${LIBPAM}
LDADD+= -lpam
.endif

.include <bsd.prog.mk>
