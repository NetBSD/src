# $NetBSD: Makefile,v 1.5 2008/10/18 14:41:49 scw Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR} ${.CURDIR}/../wpa_supplicant

PROG=	wpa_passphrase
SRCS=	wpa_passphrase.c sha1.c md5.c
CPPFLAGS+= -DINTERNAL_MD5 -DINTERNAL_SHA1


MAN=	wpa_passphrase.8

.include <bsd.prog.mk>
