#	from: @(#)Makefile	8.1 (Berkeley) 6/2/93
#	$Id: Makefile,v 1.12 1994/08/03 20:49:28 deraadt Exp $

PROG=	ps
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBMATH} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
