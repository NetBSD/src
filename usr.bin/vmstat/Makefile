#	from: @(#)Makefile	5.7 (Berkeley) 4/24/91
#	$Id: Makefile,v 1.4 1994/01/28 00:48:43 cgd Exp $

PROG=	vmstat
CFLAGS+=-I${DESTDIR}/sys
MAN8=	vmstat.0
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
