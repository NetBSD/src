#	$NetBSD: Makefile,v 1.1.6.2 2002/06/23 17:50:02 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DEXEC_ELF64

KMOD=	exec_linux_elf

SRCS=	lkminit_exec.c

.if ${MACHINE} == "alpha" || ${MACHINE} == "sparc64"
SRCS+=	linux_exec_elf64.c
.else
SRCS+=	linux_exec_elf32.c
.endif

# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=linux_elf_emul_find			\
		-Dstackgap_init=linux_stackgap_init 		\
		-Demul_flags_translate=linux_elf_emul_flags_translate \
		-Dstackgap_alloc=linux_elf_stackgap_alloc 	\
		-Dcompat_offseterr=linux_elf_compat_offseterr
.PATH:	$S/compat/common
SRCS+=	compat_util.c

.include <bsd.kmod.mk>
