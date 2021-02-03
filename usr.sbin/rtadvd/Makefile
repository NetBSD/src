# $NetBSD: Makefile,v 1.26 2021/02/03 06:50:07 roy Exp $

WARNS?=	4

.include <bsd.own.mk>

USE_FORT?=	yes	# network server

RUMPPRG=	rtadvd
SRCS=		rtadvd.c advcap.c if.c config.c timer.c dump.c
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

COPTS.if.c+=		${GCC_NO_STRINGOP_TRUNCATION}
COPTS.config.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
