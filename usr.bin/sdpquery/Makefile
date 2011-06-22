# $NetBSD: Makefile,v 1.7 2011/06/22 02:49:44 mrg Exp $

USE_FORT?= yes	# network client

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c command.c print.c

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.sdpquery.c+=	-Wno-error
COPTS.print.c+=	-Wno-error
.endif
