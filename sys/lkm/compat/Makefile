# $NetBSD: Makefile,v 1.3.6.1 1997/03/12 21:24:05 is Exp $

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
