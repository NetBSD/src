# $NetBSD: Makefile,v 1.5 1997/02/18 15:31:51 scottr Exp $

#
# XXX  the compat LKMs are currently broken.
#
.if 0
.if (${MACHINE} == "i386")
SUBDIR+=	freebsd linux
.endif

.if (${MACHINE} == "sparc")
SUBDIR+=	sunos svr4
.endif

.if (${MACHINE_ARCH} == "m68k")
SUBDIR+=	sunos
.endif
.endif


.include <bsd.subdir.mk>
