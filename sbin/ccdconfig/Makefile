#	$NetBSD: Makefile,v 1.4 1997/09/15 12:45:50 lukem Exp $

PROG=	ccdconfig
MAN=	ccdconfig.8

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
