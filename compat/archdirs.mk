#	$NetBSD: archdirs.mk,v 1.5 2014/08/10 23:40:33 matt Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "sparc64"
ARCHDIR_SUBDIR=	sparc64/sparc
.endif

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR=	amd64/i386
.endif

.if (${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb")
ARCHDIR_SUBDIR=	arm/eabi
.endif

.if (${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb")
ARCHDIR_SUBDIR=	arm/oabi
.endif

.if (${MACHINE_ARCH} == "earmhf" || ${MACHINE_ARCH} == "earmhfeb")
ARCHDIR_SUBDIR=	arm/oabi arm/eabi
.endif

.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el")
ARCHDIR_SUBDIR=	mips64/64 mips64/o32
.endif

.if ${MACHINE_ARCH} == "powerpc64"
ARCHDIR_SUBDIR= powerpc64/powerpc
.endif

.if (${MACHINE_ARCH} == "aarch64")
ARCHDIR_SUBDIR+= arm/eabi
#ARCHDIR_SUBDIR+= arm/eabihf
ARCHDIR_SUBDIR+= arm/oabi
.else if (${MACHINE_ARCH} == "aarch64eb")
ARCHDIR_SUBDIR= arm/eabi
.endif
