#	$NetBSD: Makefile,v 1.2 2005/02/22 14:40:02 peter Exp $
#	$OpenBSD: Makefile,v 1.4 2003/11/20 23:23:09 avsm Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

PROG=	ftp-proxy
SRCS=	ftp-proxy.c getline.c util.c
MAN=	ftp-proxy.8

WARNS?=	2

CPPFLAGS+=-I${NETBSDSRCDIR}/sys

.include <bsd.own.mk>

# IP Filter support
.if (${MKIPFILTER} != "no")
SRCS+=	ipf.c
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/ipf -DWITH_IPF
.endif

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

BINDIR=	/usr/libexec

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/ftp-proxy
