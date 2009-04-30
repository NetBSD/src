# $NetBSD: Makefile,v 1.3 2009/04/30 04:59:14 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= netpgp
SRCS+= compress.c create.c crypto.c
SRCS+= keyring.c misc.c netpgp.c openssl_crypto.c packet-parse.c
SRCS+= packet-print.c packet-show.c reader.c signature.c
SRCS+= symmetric.c validate.c writer.c
CPPFLAGS+= -I${EXTDIST}/include
MAN= libnetpgp.3
WARNS=3

EXTDIST=${NETBSDSRCDIR}/crypto/external/bsd/netpgp/dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/lib

INCS+= netpgp.h
INCSDIR=/usr/include

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/lib/libcrypto
LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>
