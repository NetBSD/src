#	$NetBSD: archdirs.mk,v 1.2.2.1 2013/06/23 06:26:13 tls Exp $

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
