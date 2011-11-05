#	$NetBSD: Makefile,v 1.1 2011/11/05 22:39:13 christos Exp $

TREDIST=	${.CURDIR}/../dist

.PATH: ${TREDIST}/src

CPPFLAGS+=	-I${TREDIST}/lib -I${.CURDIR}/../libtre
CPPFLAGS+=	-DHAVE_CONFIG_H=1

BINDIR=		/usr/bin
PROG=		agrep
SRCS=		agrep.c
LDADD+=		-ltre
DPADD+=		${LIBTRE}

WARNS=	4

.include <bsd.prog.mk>
