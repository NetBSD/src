# $NetBSD: Makefile,v 1.6 2007/06/09 17:33:26 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit
DPADD+=${LIBEDIT}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
