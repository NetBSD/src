#	from: @(#)Makefile	8.1 (Berkeley) 6/2/93
#	$Id: Makefile,v 1.11 1994/07/03 07:25:57 deraadt Exp $

PROG=	ps
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
CFLAGS+=-I${DESTDIR}/sys
DPADD=	${LIBMATH} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
