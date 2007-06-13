# $NetBSD: Makefile,v 1.7 2007/06/13 07:33:00 he Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit -ltermcap
DPADD+=${LIBEDIT} ${LIBTERMCAP}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
