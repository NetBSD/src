# $NetBSD: Makefile,v 1.5 2023/06/22 22:39:23 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kpasswd

PROG= kpasswd
SRCS= kpasswd.c

PROGDPLIBS+= ${DPLIBKAFS} ${DPLIBSL} ${DPLIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= ${KRB5LDADD}
DPADD+= ${KRB5DPADD}
