#	$NetBSD: Makefile,v 1.9 1997/05/06 20:45:54 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/w ${BINDIR}/uptime

.PATH.c: ${.CURDIR}/../../bin/ps

.include <bsd.prog.mk>
