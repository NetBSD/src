#	from: @(#)Makefile	5.2 (Berkeley) 5/13/90
#	$Id: Makefile,v 1.2 1993/07/30 22:12:30 mycroft Exp $

PROG=	lpc
CFLAGS+=-I${.CURDIR}/../common_source
MAN8=	lpc.0
SRCS=	lpc.c cmds.c cmdtab.c startdaemon.c common.c printcap.c
BINGRP=	daemon
BINMODE=2555
.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
