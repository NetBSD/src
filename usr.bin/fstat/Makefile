#	from: @(#)Makefile	5.5 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.4 1994/01/28 00:45:44 cgd Exp $

PROG=	fstat
SRCS=	fstat.c devname.c
.PATH:	${.CURDIR}/../../bin/ps
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
