#	$Id: Makefile,v 1.6 1994/11/15 07:20:52 glass Exp $

PROG=	pppstats
SRCS=	pppstats.c
CFLAGS+= -DPPP
LDADD=	-lkvm
DPADD=	${LIBKVM}
MAN8=	pppstats.0
MLINKS= pppstats.8 slstats.8
BINMODE=2555
BINGRP=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
