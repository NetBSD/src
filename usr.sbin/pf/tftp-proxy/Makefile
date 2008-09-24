#	$NetBSD: Makefile,v 1.2.2.2 2008/09/24 04:54:48 wrstuden Exp $
#	$OpenBSD: Makefile,v 1.1 2005/12/28 19:07:07 jcs Exp $

PROG=	tftp-proxy
SRCS=	tftp-proxy.c filter.c
MAN=	tftp-proxy.8

WARNS?=	2

CPPFLAGS+=-I${NETBSDSRCDIR}/sys

BINDIR= /usr/libexec

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/tftp-proxy
