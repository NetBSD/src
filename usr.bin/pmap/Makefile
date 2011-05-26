#	$NetBSD: Makefile,v 1.5 2011/05/26 12:56:33 joerg Exp $

USE_FORT?=	yes	# setgid
PROG=		pmap
SRCS=		main.c pmap.c
BINGRP=		kmem
BINMODE=	2555
LDADD=		-lkvm
DPADD=		${LIBKVM}

CWARNFLAGS.clang+=	-Wno-format-extra-args

.include <bsd.prog.mk>
