#	$NetBSD: Makefile,v 1.6 2015/12/14 03:15:10 christos Exp $

USE_FORT?=	yes	# setgid
PROG=		pmap
SRCS=		main.c pmap.c
BINGRP=		kmem
BINMODE=	2555
LDADD=		-lutil -lkvm
DPADD=		${LIBUTIL} ${LIBKVM}

CWARNFLAGS.clang+=	-Wno-format-extra-args

.include <bsd.prog.mk>
