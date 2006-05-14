# $NetBSD: Makefile,v 1.1 2006/05/14 19:51:15 dyoung Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_passphrase
SRCS=	wpa_passphrase.c sha1.c md5.c

MAN=	wpa_passphrase.8

.include <bsd.prog.mk>
