# $NetBSD: Makefile,v 1.5 2005/02/19 16:58:26 thorpej Exp $

WARNS=	0	# Will be fixed later

.include <bsd.own.mk>

PROG=	setkey
SRCS=	setkey.c parse.y token.l
MAN=	setkey.8

YHEADER=parse.h

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools

CPPFLAGS+= -I${DIST}/src/setkey -I${DIST}/src/libipsec
CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libipsec -I.
CPPFLAGS+= -DIPSEC_DEBUG -DHAVE_CONFIG_H

LDADD+= -ll -ly -lipsec
DPADD+=	${LIBL} ${LIBY} ${LIBIPSEC}

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH: ${DIST}/src/setkey 

.include <bsd.prog.mk>
