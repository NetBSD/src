#	$NetBSD: Makefile,v 1.9 2000/01/22 10:22:55 mjl Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rshd
MAN=	rshd.8

CFLAGS+=-DLOGIN_CAP
LDADD+= -lutil

.include <bsd.prog.mk>
