#	$NetBSD: Makefile,v 1.7 2005/01/20 16:39:23 xtraeme Exp $

PROG=	ccdconfig
MAN=	ccdconfig.8 ccd.conf.5

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
