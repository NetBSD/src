#	$NetBSD: Makefile,v 1.9.4.1 1999/12/27 18:30:24 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifconfig
MAN=	ifconfig.8

CPPFLAGS+=-DINET6

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>
