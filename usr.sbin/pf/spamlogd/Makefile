#	$NetBSD: Makefile,v 1.2 2004/11/11 12:03:54 yamt Exp $
#	$OpenBSD: Makefile,v 1.2 2004/02/27 19:41:39 david Exp $

PROG=	spamlogd
SRCS=	spamlogd.c
MAN=	spamlogd.8

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/libexec/spamd

BINDIR=	/usr/libexec

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamlogd
