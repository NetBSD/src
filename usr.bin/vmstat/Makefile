#	from: @(#)Makefile	5.7 (Berkeley) 4/24/91
#	$Id: Makefile,v 1.2 1993/07/30 22:30:07 mycroft Exp $

PROG=	vmstat
CFLAGS+=-I/sys
MAN8=	vmstat.0
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
