#	from: @(#)Makefile	5.4 (Berkeley) 10/26/90
#	$Id: Makefile,v 1.4 1994/01/28 00:52:35 cgd Exp $

PROG=	lpd
CFLAGS+=-I${.CURDIR}/../common_source
MAN8=	lpd.0
SRCS=	lpd.c printjob.c recvjob.c displayq.c rmjob.c startdaemon.c \
	lpdchar.c common.c printcap.c
.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
