#	$Id: Makefile,v 1.4 1994/01/28 00:56:02 cgd Exp $

PROG=	pppstats
SRCS=	pppstats.c
LDADD=	-lkvm
DPADD=	${LIBKVM}
NOMAN=	noman
BINMODE=2555
BINGRP=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
