# $NetBSD: Makefile,v 1.1.2.1 2012/05/06 18:07:01 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB= netpgp
SRCS+= compress.c create.c crypto.c
SRCS+= keyring.c misc.c netpgp.c openssl_crypto.c packet-parse.c
SRCS+= packet-print.c packet-show.c reader.c signature.c
SRCS+= symmetric.c validate.c writer.c
SRCS+= ssh2pgp.c bufgap.c
CPPFLAGS+= -I${.CURDIR} -I${EXTDIST}/include
CPPFLAGS+= -I${EXTDIST}/src/libcipher
CPPFLAGS+= -I${EXTDIST}/src/libbn
CPPFLAGS+= -I${EXTDIST}/src/librsa
CPPFLAGS+= -I${EXTDIST}/src/libverify
MAN= libnetpgp.3
WARNS?= 4

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/libnetpgp

INCS+= netpgp.h
INCSDIR=/usr/include/netpgp

LDADD+=	-lmj -lz -lbz2
#LIBDPLIBS+=	mj	${.CURDIR}/../libmj
#LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto
#LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
#LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>
