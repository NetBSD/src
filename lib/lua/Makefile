#	$NetBSD: Makefile,v 1.6.24.1 2018/05/21 04:35:56 pgoyette Exp $

.include <bsd.own.mk>

.if ${MKPIC} != "no"
# No support for shared libraries and pic code.
SUBDIR+=	bozohttpd gpio sqlite syslog
.endif

.include <bsd.subdir.mk>
