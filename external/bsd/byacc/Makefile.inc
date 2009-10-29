#	$NetBSD: Makefile.inc,v 1.1 2009/10/29 00:56:35 christos Exp $

WARNS=4

.include <bsd.own.mk>

BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/byacc/dist

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}

.PATH: ${IDIST}
