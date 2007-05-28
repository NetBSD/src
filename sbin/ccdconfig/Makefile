#	$NetBSD: Makefile,v 1.8 2007/05/28 12:06:22 tls Exp $

USE_FORT?=yes	# setgid

PROG=	ccdconfig
MAN=	ccdconfig.8 ccd.conf.5

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
