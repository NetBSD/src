#	$NetBSD: Makefile,v 1.10 2010/02/03 15:34:47 roy Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpc
MAN=	lpc.8
SRCS=	lpc.c cmds.c cmdtab.c
BINGRP=	daemon
BINMODE=2555

LDADD+= -ledit -lterminfo
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <bsd.prog.mk>
