# $NetBSD: Makefile,v 1.5 2009/07/20 17:29:08 christos Exp $

LIB=	pam_afslog
SRCS=	pam_afslog.c
MAN=	pam_afslog.8

LIBDPLIBS+=	krb5	${.CURDIR}/../../../libkrb5 \
		kafs	${.CURDIR}/../../../libkafs \
		asn1	${.CURDIR}/../../../libasn1 \
		roken	${.CURDIR}/../../../libroken \
		com_err	${.CURDIR}/../../../libcom_err \
		crypt	${.CURDIR}/../../../libcrypt \
		crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto

.include "${.CURDIR}/../mod.mk"
