# $NetBSD: Makefile,v 1.1.1.1.2.1 2011/02/17 11:57:14 bouyer Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB=saslc
SRCS=
SRCS+= buffer.c
SRCS+= crypto.c
SRCS+= dict.c
SRCS+= error.c
SRCS+= list.c
SRCS+= mech.c
SRCS+= mech_anonymous.c
SRCS+= mech_crammd5.c
SRCS+= mech_digestmd5.c
SRCS+= mech_external.c
.if (${MKKERBEROS} != "no")
SRCS+= mech_gssapi.c
.endif
SRCS+= mech_login.c
SRCS+= mech_plain.c
SRCS+= msg.c
SRCS+= parser.c
SRCS+= saslc.c
SRCS+= xsess.c

CPPFLAGS+=-I${EXTDIST}/include
MAN=libsaslc.3
WARNS?=4

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src ${EXTDIST}/man

INCS+= saslc.h
INCSDIR=/usr/include

LIBDPLIBS+= ssl ${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libssl
.if (${MKKERBEROS} != "no")
LIBDPLIBS+= gssapi ${NETBSDSRCDIR}/lib/libgssapi
.endif

.include <bsd.lib.mk>
