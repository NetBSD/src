#	$NetBSD: Makefile,v 1.9 2006/01/12 17:53:03 garbled Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpc
MAN=	lpc.8
SRCS=	lpc.c cmds.c cmdtab.c
BINGRP=	daemon
BINMODE=2555

LDADD+= -ledit -ltermcap
DPADD+= ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
