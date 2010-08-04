# $NetBSD: Makefile,v 1.1 2010/08/04 17:12:34 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c os_unix.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit -lterminfo
DPADD+=${LIBEDIT} ${LIBTERMINFO}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
