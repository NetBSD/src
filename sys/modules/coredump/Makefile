#	$NetBSD: Makefile,v 1.9 2023/06/03 09:09:14 lukem Exp $

.include "../Makefile.inc"

KMOD=	coredump

.PATH:	${S}/kern
SRCS=	kern_core.c core_netbsd.c

# Include only the primary object format.  If both elf64 and elf32 are
# supported, this will pick up elf64 and elf32 will be available in
# the compat_netbsd32_coredump module

.if ${OBJECT_FMTS:Melf64} != ""
SRCS+=		core_elf64.c
CPPFLAGS+=	-DEXEC_ELF64
.elif ${OBJECT_FMTS:Melf32} != ""
SRCS+=		core_elf32.c
CPPFLAGS+=	-DEXEC_ELF32
.endif

.PATH:	${S}/uvm
SRCS+=	uvm_coredump.c

.include "../ptrace_common/machdep.mk"
SRCS+=	core_machdep.c

WARNS=	3

COPTS.kern_core.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
