#	$NetBSD: Makefile,v 1.6 1997/05/08 21:12:26 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpq
CFLAGS+=-I${.CURDIR}/../common_source
SRCS=	lpq.c displayq.c common.c
BINOWN=	root
BINGRP=	daemon
BINMODE=6555
BINDIR=	/usr/bin
.PATH:	${.CURDIR}/../common_source

.include <bsd.prog.mk>
