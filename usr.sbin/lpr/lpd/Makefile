#	$NetBSD: Makefile,v 1.11.10.1 2000/06/22 18:01:02 minoura Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpd
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c lpdchar.c key.c modes.c ttcompat.c rcmd.c

# the ugly hack is to correctly include lib/libc/include/pathnames.h,
# not usr.sbin/lpr/common_source/pathnames.h
rcmd.o: rcmd.c
	${CC} ${CFLAGS} -I${.CURDIR}/../../../lib/libc/include -DINET6 \
	    ${CPPFLAGS} -c ${.IMPSRC}

.PATH:	${.CURDIR}/../../../lib/libc/net

.include <bsd.prog.mk>
