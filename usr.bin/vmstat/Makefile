#	$NetBSD: Makefile,v 1.18 1999/03/24 05:51:32 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat

SRCS=	dkstats.c vmstat.c
MAN=	vmstat.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

# XXX
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
