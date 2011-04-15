# $NetBSD: Makefile,v 1.6 2011/04/15 18:05:45 elric Exp $

LIB=	pam_afslog
SRCS=	pam_afslog.c
MAN=	pam_afslog.8

LIBDPLIBS+= \
	krb5	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libkrb5 \
	kafs	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libkafs \
	asn1	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libasn1 \
	roken	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libroken \
	com_err	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libcom_err \
	crypt	${.CURDIR}/../../../libcrypt \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto

.include "${.CURDIR}/../mod.mk"
