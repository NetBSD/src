#	$NetBSD: Makefile,v 1.4 1997/06/29 18:28:18 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

CFLAGS+=-Wall -Wstrict-prototypes -Wmissing-prototypes
PROG=	wall
BINGRP=	tty
BINMODE=2555
DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
