# $NetBSD: Makefile,v 1.17 2017/10/10 19:30:06 christos Exp $

.include <bsd.own.mk>

USE_FORT?=	yes	# setuid
RUMPPRG=	ping6
MAN=		ping6.8

BINOWN=		root
BINMODE=	4555

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-DIPSEC

LDADD+=		-lipsec -lm
DPADD+=		${LIBIPSEC} ${LIBM}

.if (${MKRUMP} != "no")
CPPFLAGS+= 	-DRUMP_ACTION
LDADD.rump+=	-lrumpres
DPADD.rump+=	${LIBRUMPRES}
.endif

.include <bsd.prog.mk>
