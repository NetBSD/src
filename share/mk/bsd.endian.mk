#	$NetBSD: bsd.endian.mk,v 1.7 2004/03/13 02:36:43 christos Exp $

.if !defined(_BSD_ENDIAN_MK_)
_BSD_ENDIAN_MK_=1

.include <bsd.init.mk>

.if ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "mipsel" ||
    ${MACHINE_ARCH} == "ns32k" || \
    ${MACHINE_ARCH} == "sh5el" || \
    ${MACHINE_ARCH} == "vax"
TARGET_ENDIANNESS=	little
.elif ${MACHINE_ARCH} == "armeb" || \
      ${MACHINE_ARCH} == "hppa" || \
      ${MACHINE_ARCH} == "m68k" || \
      ${MACHINE_ARCH} == "mipseb" || \
      ${MACHINE_ARCH} == "powerpc" || \
      ${MACHINE_ARCH} == "sh5eb" || \
      ${MACHINE_ARCH} == "sparc" || \
      ${MACHINE_ARCH} == "sparc64"
TARGET_ENDIANNESS=	big
.else
TARGET_ENDIANNESS=	unknown
.endif

.endif  # !defined(_BSD_ENDIAN_MK_)
