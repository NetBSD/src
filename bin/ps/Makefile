#	$NetBSD: Makefile,v 1.21 1998/08/21 19:39:41 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=	ps
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
CFLAGS+=-Wno-format-y2k
DPADD=	${LIBM} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

# XXX
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
