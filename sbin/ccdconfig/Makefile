#	$NetBSD: Makefile,v 1.3 1997/07/20 05:01:14 thorpej Exp $

PROG=	ccdconfig
MAN=	ccdconfig.8

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

WARNS=1

.include <bsd.prog.mk>
