#	$NetBSD: Makefile,v 1.1 2004/11/11 11:54:51 yamt Exp $
#	$OpenBSD: Makefile,v 1.7 2004/02/26 07:28:55 beck Exp $

PROG=	spamd
SRCS=	spamd.c sdl.c grey.c
MAN=	spamd.8

WARNS?=	1

# NetBSD doesn't have syslog_r.
CPPFLAGS+=	-D'openlog_r(ident, logopt, fac, data)=/*nothing*/'
CPPFLAGS+=	-D'syslog_r(pri, data, ...)=syslog((pri), __VA_ARGS__)'

BINDIR=	/usr/libexec

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd
