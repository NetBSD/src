# $NetBSD: Makefile,v 1.2 2023/05/09 20:14:37 christos Exp $

WARNS?=	0	# XXX third-party program, many issues
NOCLANGERROR=	# defined

.include <bsd.own.mk>

PROG=	plainrsa-gen
SRCS=	plainrsa-gen.c plog.c crypto_openssl.c logger.c vmbuf.c misc.c \
	rsalist.c sockmisc.c genlist.c prsa_tok.c prsa_par.c
MAN=	plainrsa-gen.8

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
CPPFLAGS+=	-I${DIST}/src/racoon -I${DIST}/src/racoon/missing
CPPFLAGS+=	-I${DIST}/src/libipsec
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libipsec
CPPFLAGS+=	-DHAVE_CONFIG_H -DNOUSE_PRIVSEP
.if ${HAVE_OPENSSL} < 11
CPPFLAGS+=	-DHAVE_OPENSSL_RC5_H
CPPFLAGS+=	-DOPENSSL_API_COMPAT=0x10100000L
.endif

.PATH:  ${DIST}/src/racoon

LDADD+= -lcrypto -lipsec
DPADD+= ${LIBCRYPTO} ${LIBIPSEC}

COPTS.plainrsa-gen.c+= -Wno-error=deprecated-declarations
COPTS.crypto_openssl.c+= -Wno-error=deprecated-declarations
COPTS.rsalist.c+= -Wno-error=deprecated-declarations
COPTS.prsa_par.c+= -Wno-error=deprecated-declarations

LPREFIX=prsa
YPREFIX=prsa
YFLAGS=-d

prsa_tok.c: prsa_par.c

.include <bsd.prog.mk>

