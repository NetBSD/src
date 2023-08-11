# $NetBSD: Makefile,v 1.3.10.1 2023/08/11 13:40:06 martin Exp $

USE_FORT?= yes	# cryptographic software

NOLINT=

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/ntlm ${HEIMBASE}/include

LIB=		heimntlm

LIBDPLIBS+=	asn1	${.CURDIR}/../libasn1 \
		com_err	${.CURDIR}/../libcom_err \
		crypto	${SSLBASE}/lib/libcrypto \
		krb5	${.CURDIR}/../libkrb5 \
		roken	${.CURDIR}/../libroken \
		wind	${.CURDIR}/../libwind

HEIMSRCS= ntlm_err.et ntlm.c

CPPFLAGS+=				\
	-I${HEIMDIST}/lib

INCS=		heimntlm.h heimntlm-protos.h ${COMPILE_ET_INCS}

INCSDIR=	/usr/include/krb5

COPTS.ntlm.c+= -Wno-error=deprecated-declarations

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.lib.mk>
