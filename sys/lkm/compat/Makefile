# $NetBSD: Makefile,v 1.7 2000/12/08 20:20:23 jdolecek Exp $

# XXX for now, only Linux on i386 was actually tested, others might
# need further work; compile stuff on everything, but only install
# compat_linux on i386
.if make(install)
.if (${MACHINE} == "i386")
SUBDIR+=	linux
.endif
.else # !make(install)

.if (${MACHINE} == "alpha")
SUBDIR+=	linux osf1
.endif

.if (${MACHINE} == "hp300")
SUBDIR+=	hpux
.endif

.if (${MACHINE} == "i386")
SUBDIR+=	aout freebsd ibcs2 linux svr4
.endif

.if (${MACHINE_ARCH} == "m68k")
SUBDIR+=	aoutm68k linux sunos svr4
.endif

.if (${MACHINE_ARCH} == "mipsle" || ${MACHINE_ARCH} == "mipsbe")
SUBDIR+=	ultrix
.endif

.if (${MACHINE} == "sparc")
SUBDIR+=	aout sunos svr4
.endif

.if (${MACHINE_ARCH} == "sparc64")
SUBDIR+=	netbsd32 sunos svr4
.endif

.if (${MACHINE_ARCH} == "vax")
SUBDIR+=	ibcs2 ultrix
.endif

.endif # !make(install)

.include <bsd.subdir.mk>
