#	$NetBSD: Makefile,v 1.5 1997/05/06 20:46:19 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lprm
CFLAGS+=-I${.CURDIR}/../common_source
SRCS=	lprm.c rmjob.c startdaemon.c common.c
BINOWN=	root
BINGRP=	daemon
BINMODE=6555
BINDIR=	/usr/bin
.PATH.c: ${.CURDIR}/../common_source

.include <bsd.prog.mk>
