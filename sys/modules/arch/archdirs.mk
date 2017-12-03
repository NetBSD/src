#	$NetBSD: archdirs.mk,v 1.1.12.2 2017/12/03 11:38:49 jdolecek Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR=	x86/amd64-xen
.endif

.if ${MACHINE} == "i386"
ARCHDIR_SUBDIR=	x86/i386-xen x86/i386pae-xen
.endif

.if ${MACHINE_ARCH} == "powerpc"
ARCHDIR_SUBDIR=	powerpc/powerpc-4xx powerpc/powerpc-booke
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
ARCHDIR_SUBDIR= mips/mips-n32
.endif
