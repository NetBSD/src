#	$NetBSD: Makefile,v 1.15 2002/08/12 18:03:41 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lpd
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c lpdchar.c key.c modes.c ttcompat.c rcmd.c

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

# the ugly hack is to correctly include lib/libc/include/pathnames.h,
# not usr.sbin/lpr/common_source/pathnames.h
rcmd.o: rcmd.c
	${CC} ${CFLAGS} -I${.CURDIR}/../../../lib/libc/include -DINET6 \
	    ${CPPFLAGS} -c ${.IMPSRC}

.PATH:	${.CURDIR}/../../../lib/libc/net

.include <bsd.prog.mk>
