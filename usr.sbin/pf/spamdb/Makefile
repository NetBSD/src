#	$NetBSD: Makefile,v 1.1 2004/11/11 11:54:51 yamt Exp $
#	$OpenBSD: Makefile,v 1.2 2004/02/27 19:41:39 david Exp $

PROG=	spamdb
SRCS=	spamdb.c
MAN=	spamdb.8

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/libexec/spamd

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/usr.sbin/spamdb
