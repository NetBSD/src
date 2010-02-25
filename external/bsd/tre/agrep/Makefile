#	$NetBSD: Makefile,v 1.1 2010/02/25 08:06:43 agc Exp $

TREDIST=	${.CURDIR}/../dist

.PATH: ${TREDIST}/src

CPPFLAGS+=	-I${TREDIST}/lib -I${.CURDIR}/../libtre
CPPFLAGS+=	-DHAVE_CONFIG_H=1

PROG=		agrep
SRCS=		agrep.c
LDADD+=		-ltre

WARNS=	4

.include <bsd.prog.mk>
