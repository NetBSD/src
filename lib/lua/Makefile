#	$NetBSD: Makefile,v 1.3 2011/10/10 20:43:00 christos Exp $

.include <bsd.own.mk>

.if ${MACHINE_ARCH} != "m68000"
# No support for shared libraries and pic code.
SUBDIR+=	gpio
.endif

.include <bsd.subdir.mk>
