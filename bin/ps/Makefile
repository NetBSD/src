#	@(#)Makefile	5.5 (Berkeley) 4/23/91
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	ps
SRCS=	devname.c keyword.c nlist.c print.c ps.c
CFLAGS+=-I/sys
DPADD=	${LIBMATH} ${LIBUTIL}
LDADD=	-lm -lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
