#	$NetBSD: Makefile,v 1.3 2004/07/01 01:41:16 kleink Exp $
#	$OpenBSD: Makefile,v 1.4 2003/11/20 23:23:09 avsm Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

PROG=	ftp-proxy
SRCS=	ftp-proxy.c getline.c util.c ipf.c pf.c
MAN=	ftp-proxy.8

WARNS?=	2

CPPFLAGS+=-I${NETBSDSRCDIR}/sys
CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/ftp-proxy
