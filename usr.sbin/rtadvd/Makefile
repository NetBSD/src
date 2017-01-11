# $NetBSD: Makefile,v 1.20 2017/01/11 12:19:44 joerg Exp $

WARNS?=	4

.include <bsd.own.mk>

USE_FORT?=	yes	# network server

RUMPPRG=	rtadvd
SRCS=		rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c
MAN=		rtadvd.8 rtadvd.conf.5

CPPFLAGS+=	-DINET6
LDADD+=		-lutil
DPADD+=		${LIBUTIL}

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf
.endif

.PATH:	${.CURDIR}/../../lib/libc/net
RUMPSRCS=	getifaddrs.c getnameinfo.c if_indextoname.c if_nametoindex.c
.if (${MKRUMP} != "no")
CPPFLAGS+=	-DRUMP_ACTION
.endif

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

.include <bsd.prog.mk>
