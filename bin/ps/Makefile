#	@(#)Makefile	5.5 (Berkeley) 4/23/91
#
#	$Header: /cvsroot/src/bin/ps/Makefile,v 1.3 1993/03/23 00:26:23 cgd Exp $

PROG=	ps
SRCS=	devname.c keyword.c nlist.c print.c ps.c
CFLAGS+=-I/sys
DPADD=	${LIBMATH} ${LIBUTIL}
LDADD=	-lm -lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
