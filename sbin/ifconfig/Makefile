#	$NetBSD: Makefile,v 1.13 2001/04/28 00:00:06 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifconfig
MAN=	ifconfig.8

CPPFLAGS+=-DINET6

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>
