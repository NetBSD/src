#	$NetBSD: Makefile,v 1.14 2002/11/30 03:10:54 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	sysctl
MAN=	sysctl.8

CPPFLAGS+=-DINET6 -DIPSEC -I${.CURDIR} -I${NETBSDSRCDIR}/sys
#CPPFLAGS+=-DTCP6

.include <bsd.prog.mk>
