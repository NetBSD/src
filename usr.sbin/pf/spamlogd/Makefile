#	$NetBSD: Makefile,v 1.3 2005/04/12 14:44:10 jwise Exp $
#	$OpenBSD: Makefile,v 1.2 2004/02/27 19:41:39 david Exp $

PROG=	pfspamlogd
SRCS=	spamlogd.c
MAN=	pfspamlogd.8

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/libexec/spamd

BINDIR=	/usr/libexec

pfspamlogd.8:	spamlogd.8
	cp $> $@

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamlogd
