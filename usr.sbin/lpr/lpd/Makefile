#	$NetBSD: Makefile,v 1.8 1995/11/15 22:49:49 pk Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpd
CFLAGS+=-I${.CURDIR}/../common_source
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c displayq.c rmjob.c startdaemon.c \
	lpdchar.c common.c key.c modes.c ttcompat.c
.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
