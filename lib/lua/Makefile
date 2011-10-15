#	$NetBSD: Makefile,v 1.4 2011/10/15 10:46:24 mbalmer Exp $

.include <bsd.own.mk>

.if ${MACHINE_ARCH} != "m68000"
# No support for shared libraries and pic code.
SUBDIR+=	gpio sqlite
.endif

.include <bsd.subdir.mk>
