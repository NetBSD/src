# $NetBSD: Makefile,v 1.4.10.1 2009/05/13 19:20:39 jym Exp $

USE_FORT?= yes	# network server

PROG=	sdpd
MAN=	sdpd.8
SRCS=	db.c log.c main.c record.c server.c service.c

DPADD+=	${LIBBLUETOOTH}
LDADD+=	-lbluetooth

.include <bsd.own.mk>

.if (${SDP_COMPAT:Uyes} != "no")
SRCS+=		compat.c
CPPFLAGS+=	-DSDP_COMPAT

# only needed in SDP_COMPAT
.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif
.endif

.include <bsd.prog.mk>
