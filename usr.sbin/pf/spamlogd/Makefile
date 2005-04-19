#	$NetBSD: Makefile,v 1.4 2005/04/19 08:42:54 tron Exp $
#	$OpenBSD: Makefile,v 1.2 2004/02/27 19:41:39 david Exp $

PROG=	pfspamlogd
SRCS=	spamlogd.c
MAN=	pfspamlogd.8

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/libexec/spamd

BINDIR=	/usr/libexec

pfspamlogd.8:	spamlogd.8
	cp $> $@

CLEANFILES+=	pfspamlogd.8

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamlogd
