#	$Id: Makefile,v 1.4.2.1 1994/08/26 12:45:09 mycroft Exp $

PROG=	pppstats
SRCS=	pppstats.c
LDADD=	-lkvm
DPADD=	${LIBKVM}
MAN8=	pppstats.0
BINMODE=2555
BINGRP=	kmem
BINDIR=	/usr/sbin

.include <bsd.prog.mk>
