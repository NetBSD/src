#	$NetBSD: Makefile,v 1.21.2.1 2026/11/04 23:28:53 perseant Exp $
#	@(#)Makefile	8.3 (Berkeley) 1/25/94

.include <bsd.init.mk>

USE_FORT?= yes	# network server

PROG=	mountd
SRCS=	mountd.c ipsec.c get_net.c
MAN=	exports.5 mountd.8

INETDDIR= ${.CURDIR}/../inetd

.if (${MKIPSEC} != "no")
CPPFLAGS+= -I${INETDDIR} -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}
.endif

LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>

COPTS.mountd.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.PATH: ${INETDDIR}
