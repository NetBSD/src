#	from: @(#)Makefile	5.5 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.3 1993/07/31 15:22:35 mycroft Exp $

PROG=	fstat
SRCS=	fstat.c devname.c
.PATH:	${.CURDIR}/../../bin/ps
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
