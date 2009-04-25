# $NetBSD: Makefile,v 1.2 2009/04/25 01:29:15 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= netpgp
SRCS+= compress.c create.c crypto.c
SRCS+= keyring.c misc.c netpgp.c openssl_crypto.c packet-parse.c
SRCS+= packet-print.c packet-show.c reader.c signature.c
SRCS+= symmetric.c validate.c writer.c
CPPFLAGS+= -I${EXTDIST}/include
MAN= libnetpgp.3
WARNS=0
#WARNS=4

EXTDIST=${NETBSDSRCDIR}/crypto/external/bsd/netpgp/dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/lib

INCS+= netpgp.h
INCSDIR=/usr/include

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/lib/libcrypto
LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>
