#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.4 1994/11/15 07:21:40 glass Exp $

PROG=	slstats
SRCS=	pppstats.c
.PATH:	${.CURDIR}/../pppd/pppstats
NOMAN=
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
