#	$NetBSD: Makefile,v 1.16 2002/09/18 03:54:33 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	lpd
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c lpdchar.c key.c modes.c ttcompat.c rcmd.c

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

# the ugly hack is to correctly include lib/libc/include/pathnames.h,
# not usr.sbin/lpr/common_source/pathnames.h
rcmd.o: rcmd.c
	${CC} ${CFLAGS} -I${NETBSDSRCDIR}/lib/libc/include -DINET6 \
	    ${CPPFLAGS} -c ${.IMPSRC}

.PATH:	${NETBSDSRCDIR}/lib/libc/net

.include <bsd.prog.mk>
