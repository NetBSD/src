#	$NetBSD: Makefile,v 1.13 2000/05/31 07:34:45 enami Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpd
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c lpdchar.c key.c modes.c ttcompat.c rcmd.c

# the ugly hack is to correctly include lib/libc/include/pathnames.h,
# not usr.sbin/lpr/common_source/pathnames.h
rcmd.o: rcmd.c
	${CC} -I${.CURDIR}/../../../lib/libc/include -DINET6 \
	    ${COMPILE.c:S/${CC}//1} ${.IMPSRC}

.PATH:	${.CURDIR}/../../../lib/libc/net

.include <bsd.prog.mk>
