#	$NetBSD: Makefile,v 1.13 1999/08/02 17:39:13 jdolecek Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

# XXX
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
