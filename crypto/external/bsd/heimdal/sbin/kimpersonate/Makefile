# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:40:09 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG=	kimpersonate
SRCS=	kimpersonate.c
MAN=	kimpersonate.8

PROGDPLIBS+= ${DPLIBKAFS} ${DPLIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= ${KRB5LDADD}
DPADD+= ${KRB5DPADD}
