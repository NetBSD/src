# $NetBSD: Makefile,v 1.4 1997/02/18 06:08:51 scottr Exp $

.if (${MACHINE} == "i386")
SUBDIR+=	freebsd linux
.endif

.if (${MACHINE} == "sparc")
SUBDIR+=	sunos svr4
.endif

#
# XXX  the compat_sunos LKM is currently broken.
#
.if 0
.if (${MACHINE_ARCH} == "m68k")
SUBDIR+=	sunos
.endif
.endif


.include <bsd.subdir.mk>
