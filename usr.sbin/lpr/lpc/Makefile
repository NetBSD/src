#	$NetBSD: Makefile,v 1.6 1997/05/06 20:46:15 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpc
CFLAGS+=-I${.CURDIR}/../common_source
MAN=	lpc.8
SRCS=	lpc.c cmds.c cmdtab.c startdaemon.c common.c
BINGRP=	daemon
BINMODE=2555
.PATH.c: ${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
