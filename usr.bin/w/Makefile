#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.4 1994/01/28 00:49:46 cgd Exp $

PROG=	w
SRCS=	attime.c w.c proc_compare.c
MAN1=	w.0 uptime.0
DPADD=	${LIBKVM}
LDADD=	-lc -lkvm	# XXX (for getloadavg())
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/w ${BINDIR}/uptime

.include <bsd.prog.mk>
