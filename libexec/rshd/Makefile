#	$NetBSD: Makefile,v 1.11 2000/01/31 14:20:14 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rshd
MAN=	rshd.8

CPPFLAGS+=-DLOGIN_CAP
LDADD+= -lutil

CPPFLAGS+=-DINET6

.include <bsd.prog.mk>
