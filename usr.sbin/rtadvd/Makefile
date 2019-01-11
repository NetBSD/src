# $NetBSD: Makefile,v 1.22 2019/01/11 20:41:53 christos Exp $

WARNS?=	4

.include <bsd.own.mk>

USE_FORT?=	yes	# network server

RUMPPRG=	rtadvd
SRCS=		rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c
MAN=		rtadvd.8 rtadvd.conf.5

CPPFLAGS+=	-DINET6
LDADD+=		-lutil
DPADD+=		${LIBUTIL}

LIBWRAP=	${NETBSDSRCDIR}/lib/libwrap
SRCS+=		expandm.c
.PATH.c:	${LIBWRAP}
CPPFLAGS+=	-I${LIBWRAP}


.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf
.endif

.if (${MKRUMP} != "no")
CPPFLAGS+=	-DRUMP_ACTION
LDADD.rump+=	-lrumpres
DPADD.rump+=	${LIBRUMPRES}
.endif

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

.include <bsd.prog.mk>
