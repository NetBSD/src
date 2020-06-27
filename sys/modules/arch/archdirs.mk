#	$NetBSD: archdirs.mk,v 1.5 2020/06/27 06:50:00 rin Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR=	x86/amd64-xen
.endif

.if ${MACHINE} == "i386"
ARCHDIR_SUBDIR=	x86/i386pae-xen
.endif

.if ${MACHINE_ARCH} == "powerpc"
ARCHDIR_SUBDIR=	powerpc/powerpc-ibm4xx powerpc/powerpc-booke
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
ARCHDIR_SUBDIR= mips/mips-n32
.endif
