#	$NetBSD: Makefile,v 1.21.20.1 2009/05/13 19:20:03 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

USE_FORT?= yes	# setuid

PROG=	rlogin
SRCS=	rlogin.c getport.c

.PATH.c: ${.CURDIR}/../rsh
CPPFLAGS+=-I${.CURDIR}/../rsh

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
