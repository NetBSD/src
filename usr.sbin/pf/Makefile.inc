#	$NetBSD: Makefile.inc,v 1.2 2004/11/11 12:03:53 yamt Exp $

# NetBSD doesn't have syslog_r.
CPPFLAGS+=	-D'openlog_r(ident, logopt, fac, data)=/*nothing*/'
CPPFLAGS+=	-D'syslog_r(pri, data, ...)=syslog((pri), __VA_ARGS__)'

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
