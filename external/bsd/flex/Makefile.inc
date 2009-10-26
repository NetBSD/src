#	$NetBSD: Makefile.inc,v 1.1 2009/10/26 00:49:04 christos Exp $

WARNS?=	1

.include <bsd.own.mk>

BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/flex/dist

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}

.PATH: ${IDIST}
