#	from: @(#)Makefile	5.5 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.7 1994/01/28 00:23:41 cgd Exp $

PROG=	ps
SRCS=	devname.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBM} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
