#	$NetBSD: Makefile,v 1.15.8.1 2008/01/08 22:11:48 bouyer Exp $

.if ${MACHINE} == "amd64"
SUBDIR+=	linux_elf
.endif

.if ${MACHINE} == "i386"
SUBDIR+=	freebsd_aout freebsd_elf linux_elf svr4_elf pecoff
.endif

.if ${MACHINE_ARCH} == "sparc64"
SUBDIR+=	elf netbsd32_aout netbsd32_elf sunos32_aout svr4_32_elf
.endif

# These are untested, but do compile.  Once they are tested,
# move them above.

.if !make(install)

.if ${MACHINE} == "alpha"
SUBDIR+=	linux_aout linux_elf osf1_ecoff
.endif

.if ${MACHINE} == "i386"
SUBDIR+=	aout ibcs2_coff ibcs2_elf ibcs2_xout linux_aout
.endif

.if ${MACHINE_ARCH} == "m68k"
SUBDIR+=	linux_aout linux_elf sunos_aout m68k4k
.endif

.if ${MACHINE_ARCH} == "sparc"
SUBDIR+=	aout sunos_aout svr4_elf
.endif

.if ${MACHINE_ARCH} == "vax"
SUBDIR+=	vax1k
.endif

.endif # !make(install)

.include <bsd.kinc.mk>
