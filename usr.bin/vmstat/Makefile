#	$NetBSD: Makefile,v 1.16 1998/02/09 06:25:16 mrg Exp $
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
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif
