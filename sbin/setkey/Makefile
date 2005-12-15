# $NetBSD: Makefile,v 1.8 2005/12/15 14:27:21 jmc Exp $

WARNS=  0       # Will be fixed later

.include <bsd.own.mk>

PROG=	setkey
SRCS=	setkey.c parse.c token.c
MAN=	setkey.8

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

# Don't auto-frob .y or .l files.
.l.c .y.c .y.h:
	@true
