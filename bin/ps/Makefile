#	$NetBSD: Makefile,v 1.25 1999/03/26 22:36:02 bgrayson Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=		ps
SRCS=		fmt.c keyword.c nlist.c print.c ps.c procfs_ops.c
DPADD=		${LIBM} ${LIBKVM}
LDADD=		-lm -lkvm
BINGRP=		kmem
BINMODE=	2555

CWARNFLAGS+=	-Wno-format-y2k

.include <bsd.prog.mk>

# XXX
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
