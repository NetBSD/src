#	$NetBSD: bsd.rv32.mk,v 1.3 2019/04/18 10:07:07 mrg Exp $

# Keep this out of the .ifndef section, otherwise bsd.own.mk overrides this
.if empty(LD:M-m)
LD+=			-m elf32lriscv
.endif

.ifndef _COMPAT_BSD_RV32_MK_
_COMPAT_BSD_RV32_MK_=1

MLIBDIR=		rv32
LIBGCC_MACHINE_ARCH=	riscv32
LIBC_MACHINE_ARCH=	riscv32
COMMON_MACHINE_ARCH=	riscv32
KVM_MACHINE_ARCH=	riscv32
PTHREAD_MACHINE_ARCH=	riscv32
BFD_MACHINE_ARCH=	riscv32
CSU_MACHINE_ARCH=	riscv32
CRYPTO_MACHINE_CPU=	riscv32
LDELFSO_MACHINE_CPU=	riscv32
LDELFSO_MACHINE_ARCH=	riscv32
GOMP_MACHINE_ARCH=	riscv32
XORG_MACHINE_ARCH=	riscv32

.if empty(COPTS:M-mbi)
_RV32_OPTS=		-mabi=ilp32 -march=rv32g
COPTS+=			${_RV32_OPTS}
CPUFLAGS+=		${_RV32_OPTS}
LDADD+=			${_RV32_OPTS}
LDFLAGS+=		${_RV32_OPTS}
MKDEPFLAGS+=		${_RV32_OPTS}
.endif

.include "../../Makefile.compat"

.endif
