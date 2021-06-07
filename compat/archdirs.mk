#	$NetBSD: archdirs.mk,v 1.14 2021/06/07 17:11:16 christos Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "sparc64"
ARCHDIR_SUBDIR+= sparc64/sparc
.endif

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR+= amd64/i386
.endif

.if !empty(MACHINE_ARCH:Mmips64*)
ARCHDIR_SUBDIR+= mips64/64
ARCHDIR_SUBDIR+= mips64/o32
.endif

.if !empty(MACHINE_ARCH:Mmipsn64*)
ARCHDIR_SUBDIR+= mips64/n32
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
.elif (${MACHINE_ARCH} == "aarch64eb")
ARCHDIR_SUBDIR+= arm/eabi
.endif
.endif
