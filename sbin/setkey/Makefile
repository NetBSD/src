# $NetBSD: Makefile,v 1.4 2005/02/19 16:55:05 thorpej Exp $

WARNS=	0	# Will be fixed later

.include <bsd.own.mk>

PROG=	setkey
SRCS=	setkey.c parse.y token.l
MAN=	setkey.8

YHEADER=parse.h

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
CFLAGS+=-g
CPPFLAGS+=	-I${DIST}/src/setkey -I${DIST}/src/libipsec	\
		-I${NETBSDSRCDIR}/lib/libipsec -I.		\
		-DIPSEC_DEBUG -DHAVE_CONFIG_H
LDADD+= -ll -ly -lipsec
DPADD+=	${LIBL} ${LIBY} ${LIBIPSEC}
CLEANFILES+=	setkey.cat8

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH:	${NETBSDSRCDIR}/lib/libipsec ${DIST}/src/setkey 

.include <bsd.prog.mk>
