# $NetBSD: Makefile,v 1.3 2006/05/15 02:17:43 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_passphrase
SRCS=	wpa_passphrase.c sha1.c md5.c

MAN=	wpa_passphrase.8

.include <bsd.prog.mk>
