#	from: @(#)Makefile	5.7 (Berkeley) 4/24/91
#	$Id: Makefile,v 1.5 1994/04/15 07:07:23 cgd Exp $

PROG=	vmstat
MAN8=	vmstat.0
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
