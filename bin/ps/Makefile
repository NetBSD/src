#	@(#)Makefile	5.5 (Berkeley) 4/23/91
#
#	$Header: /cvsroot/src/bin/ps/Makefile,v 1.4 1993/06/17 13:16:20 mycroft Exp $

PROG=	ps
SRCS=	devname.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBMATH} ${LIBUTIL}
LDADD=	-lm -lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
