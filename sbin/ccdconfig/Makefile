#	$NetBSD: Makefile,v 1.5 2002/06/11 13:33:48 grant Exp $

PROG=	ccdconfig
MAN=	ccdconfig.8 ccd.conf.5

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
