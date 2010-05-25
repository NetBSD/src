# $NetBSD: Makefile,v 1.9 2010/05/25 01:06:41 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= netpgp
SRCS+= compress.c create.c crypto.c
SRCS+= keyring.c misc.c netpgp.c openssl_crypto.c packet-parse.c
SRCS+= packet-print.c packet-show.c reader.c signature.c
SRCS+= symmetric.c validate.c writer.c
SRCS+= ssh2pgp.c fastctype.c bufgap.c
CPPFLAGS+= -I${.CURDIR} -I${EXTDIST}/include
MAN= libnetpgp.3
WARNS=5

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/lib

INCS+= netpgp.h
INCSDIR=/usr/include

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto
LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>
