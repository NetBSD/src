#	from: @(#)Makefile	5.5 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.6 1993/08/14 13:02:21 mycroft Exp $

PROG=	ps
SRCS=	devname.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBM} ${LIBUTIL}
LDADD=	-lm -lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
