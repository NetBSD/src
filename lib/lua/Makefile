#	$NetBSD: Makefile,v 1.4.2.1 2011/11/10 14:31:37 yamt Exp $

.include <bsd.own.mk>

.if ${MKPIC} != "no"
# No support for shared libraries and pic code.
SUBDIR+=	gpio sqlite
.endif

.include <bsd.subdir.mk>
