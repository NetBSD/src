#	$Id: Makefile,v 1.2 1993/08/18 12:54:50 brezak Exp $

PROG=	pppstats
SRCS=	pppstats.c
LDADD=	-lutil
CFLAGS+=-DKVMLIB
NOMAN=	noman
BINMODE=2555
BINGRP=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
