#	$NetBSD: Makefile,v 1.7 2018/05/03 22:51:18 sevan Exp $

.include <bsd.own.mk>

.if ${MKPIC} != "no"
# No support for shared libraries and pic code.
SUBDIR+=	bozohttpd gpio sqlite syslog
.endif

.include <bsd.subdir.mk>
