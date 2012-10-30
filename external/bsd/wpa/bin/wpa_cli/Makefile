# $NetBSD: Makefile,v 1.3.6.1 2012/10/30 18:55:58 yamt Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c common.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit -lterminfo
DPADD+=${LIBEDIT} ${LIBTERMINFO}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
