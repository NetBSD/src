# $NetBSD: Makefile,v 1.3 2000/02/26 11:44:28 itojun Exp $

PROG=	mtrace6
MAN=	mtrace6.8

BINMODE=4555
BINOWN=	root

CPPFLAGS+= -I${.CURDIR}/../pim6sd -DHAVE_GETIFADDRS

.include <bsd.prog.mk>
