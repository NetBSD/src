#	$NetBSD: Makefile,v 1.4 2008/01/02 17:23:31 yamt Exp $

USE_FORT?=	yes	# setgid
PROG=		pmap
SRCS=		main.c pmap.c
BINGRP=		kmem
BINMODE=	2555
LDADD=		-lkvm
DPADD=		${LIBKVM}

.include <bsd.prog.mk>
