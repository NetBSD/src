#	$NetBSD: Makefile,v 1.1 2004/11/11 11:54:51 yamt Exp $
#	$OpenBSD: Makefile,v 1.4 2003/11/20 23:23:09 avsm Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

PROG=	ftp-proxy
SRCS=	ftp-proxy.c getline.c util.c
MAN=	ftp-proxy.8

WARNS?=	2

CPPFLAGS+=-I${NETBSDSRCDIR}/sys

# ipfilter support
SRCS+=	ipf.c
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/ipf

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

BINDIR=	/usr/libexec

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/ftp-proxy
