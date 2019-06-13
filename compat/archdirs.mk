#	$NetBSD: archdirs.mk,v 1.12 2019/06/13 21:17:54 christos Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "sparc64"
ARCHDIR_SUBDIR+= sparc64/sparc
.endif

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR+= amd64/i386
.endif

.if !empty(MACHINE_ARCH:Mearm*)
ARCHDIR_SUBDIR+= arm/oabi
.endif

.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el")
ARCHDIR_SUBDIR+= mips64/64
ARCHDIR_SUBDIR+= mips64/o32
.endif

.if ${MACHINE_ARCH} == "powerpc64"
ARCHDIR_SUBDIR+= powerpc64/powerpc
.endif

.if ${MACHINE_ARCH} == "riscv64"
ARCHDIR_SUBDIR+= riscv64/rv32
.endif

.if ${ACTIVE_CC} == "clang"
.if (${MACHINE_ARCH} == "aarch64")
ARCHDIR_SUBDIR+= arm/eabi
ARCHDIR_SUBDIR+= arm/eabihf
ARCHDIR_SUBDIR+= arm/oabi
.elif (${MACHINE_ARCH} == "aarch64eb")
ARCHDIR_SUBDIR+= arm/eabi
.endif
.endif
