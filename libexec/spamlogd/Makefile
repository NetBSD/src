#	$NetBSD: Makefile,v 1.2 2004/11/11 11:27:34 yamt Exp $
#	$OpenBSD: Makefile,v 1.2 2004/02/27 19:41:39 david Exp $

PROG=	spamlogd
SRCS=	spamlogd.c
MAN=	spamlogd.8

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/libexec/spamd

# NetBSD doesn't have syslog_r.
CPPFLAGS+=	-D'openlog_r(ident, logopt, fac, data)=/*nothing*/'
CPPFLAGS+=	-D'syslog_r(pri, data, ...)=syslog((pri), __VA_ARGS__)'

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamlogd
