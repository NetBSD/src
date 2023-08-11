# $NetBSD: Makefile,v 1.4.8.1 2023/08/11 13:40:10 martin Exp $

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

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src/lib

INCS+= netpgp.h
INCSDIR=/usr/include

OPENSSL=${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}
LIBDPLIBS+=	mj		${.CURDIR}/../../libmj
LIBDPLIBS+=	crypto		${OPENSSL}/lib/libcrypto
LIBDPLIBS+=	z		${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2		${NETBSDSRCDIR}/lib/libbz2

COPTS.crypto.c+=	${GCC_NO_STRINGOP_OVERFLOW}
COPTS.openssl_crypto.c+=-Wno-error=deprecated-declarations
COPTS.symmetric.c+=-Wno-error=deprecated-declarations

.include <bsd.lib.mk>
