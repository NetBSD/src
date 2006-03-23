#	$NetBSD: Makefile,v 1.20 2006/03/23 23:44:15 wiz Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c getport.c
WARNS=	3

.PATH.c: ${.CURDIR}/../rsh
CPPFLAGS+=-I${.CURDIR}/../rsh

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
