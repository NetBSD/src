#	$NetBSD: Makefile,v 1.2 2004/06/30 13:29:43 darrenr Exp $
#	$OpenBSD: Makefile,v 1.4 2003/11/20 23:23:09 avsm Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

PROG=	ftp-proxy
SRCS=	ftp-proxy.c getline.c util.c ipf.c pf.c
MAN=	ftp-proxy.8

WARNS?=	2

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/ftp-proxy
