#	$NetBSD: Makefile,v 1.7.2.1 2017/01/07 08:56:58 pgoyette Exp $

USE_FORT?=	yes	# setgid
PROG=		pmap
SRCS=		main.c pmap.c
BINGRP=		kmem
BINMODE=	2555
LDADD=		-lutil -lkvm
DPADD=		${LIBUTIL} ${LIBKVM}

CWARNFLAGS.clang+=	-Wno-format-extra-args
CPPFLAGS+=	-D_KMEMUSER

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
