# $NetBSD: Makefile,v 1.25.6.1 2008/01/02 21:56:32 bouyer Exp $

.if (${MACHINE_ARCH} == "sparc64")
SUBDIR+=	netbsd32 sunos32 svr4 svr4_32
.endif

.if (${MACHINE} == "i386")
SUBDIR+=	freebsd linux svr4 pecoff
.endif

.if (${MACHINE} == "amd64")
SUBDIR+=	linux
.endif

# These are untested, but do compile.  Once they are tested,
# move them above.

.if !make(install)

.if (${MACHINE} == "alpha")
SUBDIR+=	linux osf1
.endif

.if (${MACHINE} == "i386")
SUBDIR+=	ibcs2
.endif

.if (${MACHINE_ARCH} == "m68k")
SUBDIR+=	aoutm68k sunos svr4
.endif

.if (${MACHINE_ARCH} == "mipsel")
SUBDIR+=	ultrix
.endif

.if (${MACHINE_ARCH} == "sparc")
SUBDIR+=	sunos svr4
.endif

.if (${MACHINE} == "sgimips")
SUBDIR+=	irix
.endif

.if (${MACHINE_ARCH} == "vax")
SUBDIR+=	ibcs2 ultrix
.endif

.endif # !make(install)

.include <bsd.subdir.mk>
