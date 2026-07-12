#	$NetBSD: Makefile,v 1.3 2026/07/12 15:39:07 martin Exp $
#
# Makefile for evbmips
#

MENUS_MD=	menus.md.${SYSINSTLANG}
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
NO_MBR=no

# this is a hack to mimic the #ifdef cascade in md.h to select
# kernel names - keep them in sync!
# XXX we need defines for mipsn64
.if ${MACHINE_ARCH} == mipsel || \
	${MACHINE_ARCH} == mipseb
MD_OPTIONS+=	KERNEL_NAMES_mipsel_or_mipseb
.endif
.if ${MACHINE_ARCH} == mipsel
MD_OPTIONS+=	KERNEL_NAMES_mipsel
.endif
.if ${MACHINE_ARCH} == mips64eb || \
	${MACHINE_ARCH} == mips64el || \
	${MACHINE_ARCH} == mipsn64el || \
	${MACHINE_ARCH} == mipsn64eb
MD_OPTIONS+=	KERNEL_NAMES_mipsel64_or_mipseb64
.endif
.if ${MACHINE_ARCH} == mipsn64eb || \
	${MACHINE_ARCH} == mips64eb
MD_OPTIONS+=	KERNEL_NAMES_mips64eb
.endif

.include "../../Makefile.inc"
