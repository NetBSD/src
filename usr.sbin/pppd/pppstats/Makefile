#	$Id: Makefile,v 1.7 1994/12/22 11:38:31 cgd Exp $

PROG=	pppstats
SRCS=	pppstats.c
CFLAGS+= -DPPP
LDADD=	-lkvm
DPADD=	${LIBKVM}
MAN=	pppstats.8
MLINKS= pppstats.8 slstats.8
BINMODE=2555
BINGRP=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
