# $NetBSD: Makefile,v 1.4 2008/01/26 21:50:22 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR} ${.CURDIR}/../wpa_supplicant

PROG=	wpa_passphrase
SRCS=	wpa_passphrase.c sha1.c md5.c crypto_libc.c


MAN=	wpa_passphrase.8

.include <bsd.prog.mk>
