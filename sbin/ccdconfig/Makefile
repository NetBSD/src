#	$NetBSD: Makefile,v 1.6 2005/01/20 15:49:24 xtraeme Exp $

WARNS=	2
PROG=	ccdconfig
MAN=	ccdconfig.8 ccd.conf.5

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
