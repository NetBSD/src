#	$NetBSD: Makefile,v 1.4.8.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>

PROG=	xhost

CPPFLAGS+=${X11FLAGS.CONNECTION} -DBSD44SOCKETS
CPPFLAGS+=-DHAVE_INET_ATON

.if (${USE_INET6} != "no")
CPPFLAGS+=      -DIPv6
.endif

LDADD+=	${XLIBLDADD_XMUU}
DPADD+=	${XLIBDPADD_XMUU}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
