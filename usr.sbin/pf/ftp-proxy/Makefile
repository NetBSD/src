#	$NetBSD: Makefile,v 1.2.24.1 2008/05/25 17:52:54 peter Exp $
#	$OpenBSD: Makefile,v 1.3 2006/11/26 11:31:13 deraadt Exp $

PROG=	ftp-proxy
SRCS=	ftp-proxy.c filter.c
MAN=	ftp-proxy.8

WARNS?=	2

CFLAGS+= -I${.CURDIR}
LDADD+=	-levent
DPADD+= ${LIBEVENT}

CPPFLAGS+=-I${NETBSDSRCDIR}/sys

.include <bsd.own.mk>

# IP Filter support
.if (${MKIPFILTER} != "no")
SRCS+=	ipf.c
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/ipf -DWITH_IPF
.endif

BINDIR= /usr/sbin

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/usr.sbin/ftp-proxy
