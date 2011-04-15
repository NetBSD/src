# $NetBSD: Makefile,v 1.4 2011/04/15 18:05:42 elric Exp $

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
LIBDPLIBS+= gssapi ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libgssapi
.endif

.include <bsd.lib.mk>
