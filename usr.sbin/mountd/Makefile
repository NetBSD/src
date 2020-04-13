#	$NetBSD: Makefile,v 1.19.16.1 2020/04/13 08:05:54 martin Exp $
#	@(#)Makefile	8.3 (Berkeley) 1/25/94

USE_FORT?= yes	# network server

PROG=	mountd
SRCS=	mountd.c ipsec.c get_net.c
MAN=	exports.5 mountd.8

INETDDIR= ${.CURDIR}/../inetd

CPPFLAGS+= -I${INETDDIR} -DIPSEC
LDADD+=	-lipsec -lutil
DPADD+=	${LIBIPSEC} ${LIBUTIL}

.include <bsd.prog.mk>

COPTS.mountd.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.PATH: ${INETDDIR}
