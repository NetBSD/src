#	$NetBSD: bsd.endian.mk,v 1.11 2004/03/17 19:57:49 he Exp $

.if !defined(_BSD_ENDIAN_MK_)
_BSD_ENDIAN_MK_=1

.include <bsd.init.mk>

.if ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "mipsel" || \
    ${MACHINE_ARCH} == "ns32k" || \
    ${MACHINE_ARCH} == "sh3el" || \
    ${MACHINE_ARCH} == "sh5el" || \
    ${MACHINE_ARCH} == "vax"
TARGET_ENDIANNESS=	1234
.elif ${MACHINE_ARCH} == "armeb" || \
      ${MACHINE_ARCH} == "hppa" || \
      ${MACHINE_ARCH} == "m68000" || \
      ${MACHINE_ARCH} == "m68k" || \
      ${MACHINE_ARCH} == "mipseb" || \
      ${MACHINE_ARCH} == "powerpc" || \
      ${MACHINE_ARCH} == "sh3eb" || \
      ${MACHINE_ARCH} == "sh5eb" || \
      ${MACHINE_ARCH} == "sparc" || \
      ${MACHINE_ARCH} == "sparc64"
TARGET_ENDIANNESS=	4321
.endif

.endif  # !defined(_BSD_ENDIAN_MK_)
