# $NetBSD: Makefile,v 1.3 1996/08/28 06:17:23 mrg Exp $

.if (${MACHINE} == "i386")
SUBDIR+=	freebsd linux
.endif

.if (${MACHINE} == "sparc")
SUBDIR+=	sunos svr4
.endif

.if (${MACHINE_ARCH} == "m68k")
SUBDIR+=	sunos
.endif


.include <bsd.subdir.mk>
