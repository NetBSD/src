#	from: @(#)Makefile	5.7 (Berkeley) 4/24/91
#	$Id: Makefile,v 1.3 1993/09/25 13:19:59 pk Exp $

PROG=	vmstat
CFLAGS+=-I${DESTDIR}/sys
MAN8=	vmstat.0
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
