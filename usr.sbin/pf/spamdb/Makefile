#	$NetBSD: Makefile,v 1.1.2.1 2005/04/13 16:05:49 tron Exp $
#	$OpenBSD: Makefile,v 1.2 2004/02/27 19:41:39 david Exp $

PROG=	pfspamdb
SRCS=	spamdb.c
MAN=	pfspamdb.8

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/libexec/spamd

pfspamdb.8: spamdb.8
	cp $> $@

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/usr.sbin/spamdb
