#	$NetBSD: Makefile,v 1.13 1999/03/24 05:51:32 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/w ${BINDIR}/uptime

.PATH: ${.CURDIR}/../../bin/ps

.include <bsd.prog.mk>

# XXX
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
