#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.5 1994/01/28 18:12:27 cgd Exp $

PROG=	w
SRCS=	attime.c w.c proc_compare.c
MAN1=	w.0 uptime.0
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/w ${BINDIR}/uptime

.include <bsd.prog.mk>
