# $NetBSD: Makefile,v 1.1.10.1 2014/08/19 23:45:21 tls Exp $

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
		roken	${.CURDIR}/../libroken

HEIMSRCS= ntlm_err.et ntlm.c

INCS=		heimntlm.h heimntlm-protos.h ${COMPILE_ET_INCS}

INCSDIR=	/usr/include/krb5

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.lib.mk>
