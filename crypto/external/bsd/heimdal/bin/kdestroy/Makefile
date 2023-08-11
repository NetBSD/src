# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:39:45 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kdestroy
SRCS= kdestroy.c

PROGDPLIBS+= ${DPLIBKAFS} ${DPLIBSL} ${DPLIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= ${KRB5LDADD}
DPADD+= ${KRB5DPADD}
