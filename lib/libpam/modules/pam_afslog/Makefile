# $NetBSD: Makefile,v 1.3 2008/10/27 07:48:27 mrg Exp $

LIB=	pam_afslog
SRCS=	pam_afslog.c
MAN=	pam_afslog.8

LIBDPLIBS=	krb5	${.CURDIR}/../../../libkrb5 \
		kafs	${.CURDIR}/../../../libkafs \
		asn1	${.CURDIR}/../../../libasn1 \
		roken	${.CURDIR}/../../../libroken \
		com_err	${.CURDIR}/../../../libcom_err \
		crypt	${.CURDIR}/../../../libcrypt \
		crypto	${.CURDIR}/../../../libcrypto

.include "${.CURDIR}/../mod.mk"
