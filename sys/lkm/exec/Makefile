#	$NetBSD: Makefile,v 1.3 2001/06/05 14:48:59 mrg Exp $

# XXX only linux_elf is actually tested for now; build LKMs on all platforms
# as appropriate, but only install linux_elf on i386
.if make(install)
.if ${MACHINE} == "i386"
SUBDIR+=	linux_elf
.endif
.else # !make(install)

.if ${MACHINE} == "alpha"
SUBDIR+=	linux_aout linux_elf osf1_ecoff
.endif

.if ${MACHINE} == "hp300"
SUBDIR+=	hpux_aout
.endif

.if ${MACHINE} == "i386"
SUBDIR+=	ibcs2_coff ibcs2_elf ibcs2_xout linux_aout linux_elf
SUBDIR+=	freebsd_aout freebsd_elf svr4_elf
.endif

.if ${MACHINE_ARCH} == "m68k"
SUBDIR+=	linux_aout linux_elf sunos_aout m68k4k
.endif

.if ${MACHINE_ARCH} == "sparc"
SUBDIR+=	sunos_aout svr4_elf
.endif

.if ${MACHINE_ARCH} == "sparc64"
SUBDIR+=	netbsd32_aout netbsd32_elf sunos32_aout svr4_32_elf
.endif

.if ${MACHINE_ARCH} == "vax"
SUBDIR+=	vax1k
.endif

.endif # !make(install)

.include <bsd.kinc.mk>
