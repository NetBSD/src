#	$NetBSD: Makefile,v 1.2 2003/01/08 20:25:12 atatat Exp $

PROG=		pmap
SRCS=		main.c pmap.c pmap_ld.c
BINGRP=		kmem
BINMODE=	2555
LDADD=		-lkvm
DPADD=		${LIBKVM}

.include <bsd.prog.mk>
