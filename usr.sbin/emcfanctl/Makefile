# $NetBSD: Makefile,v 1.1 2025/03/11 13:56:48 brad Exp $

PROG=	emcfanctl
SRCS=	emcfanctl.c emcfanctloutputs.c emcfanctlutil.c

LDADD+= -lmj
DPADD+= ${LIBMJ}

MAN=	emcfanctl.8

.include <bsd.prog.mk>
