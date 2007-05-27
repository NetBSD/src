# $NetBSD: Makefile,v 1.4 2007/05/27 19:46:31 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit
DPADD+=${LIBEDIT}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
