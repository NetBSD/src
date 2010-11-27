# $NetBSD: Makefile,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB=saslc
SRCS+= dict.c error.c mech_anonymous.c mech_digestmd5.c mech_gssapi.c mech.c
SRCS+= parser.c xsess.c mech_crammd5.c mech_external.c mech_login.c
SRCS+= mech_plain.c saslc.c crypto.c
CPPFLAGS+=-I${EXTDIST}/include
MAN=libsaslc.3
WARNS?=4

EXTDIST=${.CURDIR}/../dist

.PATH: ${EXTDIST}/include ${EXTDIST}/src ${EXTDIST}/man

INCS+= saslc.h
INCSDIR=/usr/include

LDADD+=-lssl -lgssapi
DPADD+=${LIBSSL}

.include <bsd.lib.mk>
