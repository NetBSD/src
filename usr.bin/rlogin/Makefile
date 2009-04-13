#	$NetBSD: Makefile,v 1.22 2009/04/13 04:37:53 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

USE_FORT?= yes	# setuid

PROG=	rlogin
SRCS=	rlogin.c getport.c

.PATH.c: ${.CURDIR}/../rsh
CPPFLAGS+=-I${.CURDIR}/../rsh

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
