#	from: @(#)Makefile	5.4 (Berkeley) 6/19/90
#	$Id: Makefile,v 1.2 1993/07/30 22:12:13 mycroft Exp $

PROG=	lprm
CFLAGS+=-I${.CURDIR}/../common_source
SRCS=	lprm.c rmjob.c startdaemon.c common.c printcap.c
BINOWN=	root
BINGRP=	daemon
BINMODE=6555
BINDIR=	/usr/bin
.PATH:	${.CURDIR}/../common_source

.include <bsd.prog.mk>
