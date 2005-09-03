# $NetBSD: Makefile,v 1.6.2.1 2005/09/03 07:03:57 snj Exp $

.include <bsd.own.mk>

PROG=	setkey
SRCS=	setkey.c parse.y token.l
MAN=	setkey.8

YHEADER=parse.h

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools

CPPFLAGS+= -I${DIST}/src/setkey -I${DIST}/src/libipsec
CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libipsec -I.
CPPFLAGS+= -DIPSEC_DEBUG -DHAVE_CONFIG_H
CPPFLAGS+= -DSADB_X_EALG_AESCBC=SADB_X_EALG_AES

LDADD+= -ll -ly -lipsec
DPADD+=	${LIBL} ${LIBY} ${LIBIPSEC}

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH: ${DIST}/src/setkey 

.include <bsd.prog.mk>
