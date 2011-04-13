# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:46 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kdc

PROG= string2key
SRCS= string2key.c
MAN=  string2key.8

CPPFLAGS+= 				\
	-I${HEIMDIST}/base		\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/wind		\
	-I${HEIMDIST}/kdc		\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMBASE}/lib/libheimntlm	\
	-I${HEIMBASE}/lib/libwind	\

LDADD+=	-lkrb5 -lcrypto -lwind ${LIBVERS}
DPADD+=	${LIBKRB5} ${LIBVERS}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
