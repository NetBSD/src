#	$NetBSD: Makefile,v 1.1.2.3 2012/10/30 18:55:57 yamt Exp $

TREDIST=	${.CURDIR}/../dist

.PATH: ${TREDIST}/src

CPPFLAGS+=	-I${TREDIST}/lib
CPPFLAGS+=	-DHAVE_CONFIG_H=1

BINDIR=		/usr/bin
PROG=		agrep
SRCS=		agrep.c
LDADD+=		-ltre
DPADD+=		${LIBTRE}

WARNS=	4

.include <bsd.prog.mk>
