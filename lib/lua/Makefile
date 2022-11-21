#	$NetBSD: Makefile,v 1.8 2022/11/21 22:01:33 christos Exp $

.include <bsd.own.mk>

.if ${MKPIC} != "no"
# No support for shared libraries and pic code.
SUBDIR+=	bozohttpd gpio libm sqlite syslog
.endif

.include <bsd.subdir.mk>
