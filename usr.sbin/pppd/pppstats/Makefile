#	$Id: Makefile,v 1.5 1994/08/26 05:13:11 paulus Exp $

PROG=	pppstats
SRCS=	pppstats.c
LDADD=	-lkvm
DPADD=	${LIBKVM}
MAN8=	pppstats.0
BINMODE=2555
BINGRP=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
