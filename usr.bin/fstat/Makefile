#	$NetBSD: Makefile,v 1.10 1998/03/04 18:07:29 mycroft Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	fstat
DPADD=	${LIBKVM}
LDADD=	-lkvm
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
