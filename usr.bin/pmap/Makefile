#	$NetBSD: Makefile,v 1.3 2007/05/28 12:06:29 tls Exp $

USE_FORT?=	yes	# setgid
PROG=		pmap
SRCS=		main.c pmap.c pmap_ld.c
BINGRP=		kmem
BINMODE=	2555
LDADD=		-lkvm
DPADD=		${LIBKVM}

.include <bsd.prog.mk>
