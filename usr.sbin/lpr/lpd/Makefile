#	from: @(#)Makefile	5.4 (Berkeley) 10/26/90
#	$Id: Makefile,v 1.3 1993/07/30 22:12:25 mycroft Exp $

PROG=	lpd
CFLAGS+=-I${.CURDIR}/../common_source
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN8=	lpd.0
SRCS=	lpd.c printjob.c recvjob.c displayq.c rmjob.c startdaemon.c \
	lpdchar.c common.c printcap.c
.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
