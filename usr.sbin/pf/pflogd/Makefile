#	$NetBSD: Makefile,v 1.1 2005/03/15 16:32:47 peter Exp $
#	$OpenBSD: Makefile,v 1.6 2003/11/20 23:23:09 avsm Exp $

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pflogd -I${NETBSDSRCDIR}/lib/libpcap

LDADD+= -lpcap -lutil
DPAPP+=	${LIBPCAP} ${LIBUTIL}

PROG=	pflogd
SRCS=	pflogd.c privsep.c privsep_fdpass.c
MAN=	pflogd.8

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pflogd
