#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.5 1995/08/02 01:27:08 paulus Exp $

PROG=	slstats
SRCS=	slstats.c
MAN=	slstats.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
