#	from: @(#)Makefile	8.1 (Berkeley) 6/2/93
#	$Id: Makefile,v 1.10 1994/05/17 08:58:29 pk Exp $

PROG=	ps
SRCS=	devname.c fmt.c keyword.c nlist.c print.c ps.c
CFLAGS+=-I${DESTDIR}/sys
DPADD=	${LIBMATH} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
