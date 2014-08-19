#	$NetBSD: Makefile,v 1.5.6.1 2014/08/20 00:02:21 tls Exp $

.include <bsd.own.mk>

.if ${MKPIC} != "no"
# No support for shared libraries and pic code.
SUBDIR+=	gpio sqlite syslog
.endif

.include <bsd.subdir.mk>
