#	$NetBSD: Makefile,v 1.8 2016/12/23 10:19:57 mrg Exp $

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
