#	$NetBSD: archdirs.mk,v 1.7 2021/08/03 09:25:44 rin Exp $

# list of subdirs used per-platform

.if ${MACHINE_ARCH} == "powerpc"
ARCHDIR_SUBDIR=	powerpc/powerpc-booke
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
ARCHDIR_SUBDIR= mips/mips-n32
.endif
