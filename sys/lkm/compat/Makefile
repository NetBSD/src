# $NetBSD: Makefile,v 1.13 2001/10/27 12:10:02 jdolecek Exp $

.if (${MACHINE} == "sparc64")
SUBDIR+=	netbsd32 sunos32 svr4 svr4_32
.endif

.if (${MACHINE} == "i386")
SUBDIR+=	freebsd linux
.endif

# These are untested, but do compile.  Once they are tested,
# move them above.

.if !make(install)

.if (${MACHINE} == "alpha")
SUBDIR+=	linux osf1
.endif

.if (${MACHINE} == "hp300")
SUBDIR+=	hpux
.endif

.if (${MACHINE} == "i386")
SUBDIR+=	aout ibcs2 svr4
.endif

.if (${MACHINE_ARCH} == "m68k")
SUBDIR+=	aoutm68k linux sunos svr4
.endif

.if (${MACHINE_ARCH} == "mipsel")
SUBDIR+=	ultrix
.endif

.if (${MACHINE} == "sparc")
SUBDIR+=	aout sunos svr4
.endif

.if (${MACHINE_ARCH} == "vax")
SUBDIR+=	ibcs2 ultrix
.endif

.endif # !make(install)

.include <bsd.subdir.mk>
