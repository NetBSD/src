#	$NetBSD: Makefile,v 1.18 2009/03/16 02:24:57 lukem Exp $
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
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
LDADD+= -lpam ${PAM_STATIC_LDADD}
.endif

.include <bsd.prog.mk>
