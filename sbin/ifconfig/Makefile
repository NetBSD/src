#	$NetBSD: Makefile,v 1.14 2002/09/20 21:21:53 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifconfig
MAN=	ifconfig.8

CPPFLAGS+=-DINET6

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
