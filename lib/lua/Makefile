#	$NetBSD: Makefile,v 1.4.2.2 2014/05/22 11:37:00 yamt Exp $

.include <bsd.own.mk>

.if ${MKPIC} != "no"
# No support for shared libraries and pic code.
SUBDIR+=	gpio sqlite syslog
.endif

.include <bsd.subdir.mk>
