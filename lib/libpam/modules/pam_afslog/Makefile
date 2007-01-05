# $NetBSD: Makefile,v 1.2.6.2 2007/01/05 14:14:53 tron Exp $

LIB=	pam_afslog
SRCS=	pam_afslog.c
MAN=	pam_afslog.8

LIBDPLIBS=	krb5	${LIB_ROOT_DIR}/libkrb5 \
		kafs	${LIB_ROOT_DIR}/libkafs \
		asn1	${LIB_ROOT_DIR}/libasn1 \
		roken	${LIB_ROOT_DIR}/libroken \
		com_err	${LIB_ROOT_DIR}/libcom_err \
		crypt	${LIB_ROOT_DIR}/libcrypt \
		crypto	${LIB_ROOT_DIR}/libcrypto

.include "${.CURDIR}/../mod.mk"
