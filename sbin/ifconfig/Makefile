#	$NetBSD: Makefile,v 1.12 2000/04/13 07:39:57 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifconfig
MAN=	ifconfig.8

CPPFLAGS+=-DINET6
CPPFLAGS+=-DHAVE_IFADDRS_H

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>
