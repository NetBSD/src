#	$NetBSD: Makefile,v 1.10 2000/01/24 14:10:56 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rshd
MAN=	rshd.8

CPPFLAGS+=-DLOGIN_CAP
LDADD+= -lutil

.include <bsd.prog.mk>
