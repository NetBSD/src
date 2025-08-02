# $NetBSD: Makefile,v 1.7.16.1 2025/08/02 05:54:53 perseant Exp $

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
	crypto	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto

.include "${.CURDIR}/../mod.mk"
