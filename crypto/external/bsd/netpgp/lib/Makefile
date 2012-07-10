# $NetBSD: Makefile,v 1.14 2012/07/10 19:38:17 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= netpgp
SRCS+= compress.c create.c crypto.c
SRCS+= keyring.c misc.c netpgp.c openssl_crypto.c packet-parse.c
SRCS+= packet-print.c packet-show.c reader.c signature.c
SRCS+= symmetric.c validate.c writer.c
SRCS+= ssh2pgp.c bufgap.c
CPPFLAGS+= -I${.CURDIR} -I${EXTDIST}/include
MAN= libnetpgp.3

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/lib

INCS+= netpgp.h
INCSDIR=/usr/include

OPENSSL=${NETBSDSRCDIR}/crypto/external/bsd/openssl
LIBDPLIBS+=	mj	${.CURDIR}/../libmj
.if (${MKCRYPTO_IDEA} != "no")
LIBDPLIBS+=	crypto_idea	${OPENSSL}/lib/libcrypto_idea
.endif  # MKCRYPTO_IDEA != no
LIBDPLIBS+=	crypto		${OPENSSL}/lib/libcrypto
LIBDPLIBS+=	z		${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2		${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>
