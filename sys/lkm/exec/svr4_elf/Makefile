#	$NetBSD: Makefile,v 1.6 2002/05/30 18:53:25 thorpej Exp $

.include "../Makefile.inc"

.if ${MACHINE} == sparc64 && ${MACHINE_ARCH} == sparc64
ELFSIZE=64
.else
ELFSIZE=32
.endif

.PATH:	$S/compat/svr4

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF${ELFSIZE} \
		-DELFSIZE=${ELFSIZE}

KMOD=	exec_svr4_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_exec_elf${ELFSIZE}.c

# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=svr4_emul_find				\
		-Dstackgap_init=svr4_stackgap_init			\
		-Demul_flags_translate=svr4_emul_flags_translate	\
		-Dstackgap_alloc=svr4_stackgap_alloc		 	\
		-Dcompat_offseterr=svr4_compat_offseterr
.PATH: $S/compat/common
SRCS+=	compat_util.c

.include <bsd.kmod.mk>
