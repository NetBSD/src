#	$NetBSD: Makefile.inc,v 1.3 2004/11/14 11:26:48 yamt Exp $

# misc OpenBSD compatibility stuffs.
#  NetBSD doesn't have syslog_r.
CPPFLAGS+=	-D'openlog_r(ident, logopt, fac, data)=/*nothing*/'
CPPFLAGS+=	-D'syslog_r(pri, data, ...)=syslog((pri), __VA_ARGS__)'
#  NetBSD doesn't have TAILQ_END.
CPPFLAGS+=	-D'TAILQ_END(h)=NULL'

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
