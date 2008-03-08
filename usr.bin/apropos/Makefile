#	$NetBSD: Makefile,v 1.12 2008/03/08 15:48:27 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

MDIST=	${NETBSDSRCDIR}/usr.bin/man

PROG=	apropos
SRCS=	apropos.c manconf.c

.PATH:	${MDIST}
CPPFLAGS+=-I${MDIST}
WARNS=	4

.include <bsd.prog.mk>
