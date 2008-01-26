# $NetBSD: Makefile,v 1.8 2008/01/26 21:50:22 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c os_unix.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit -ltermcap
DPADD+=${LIBEDIT} ${LIBTERMCAP}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
