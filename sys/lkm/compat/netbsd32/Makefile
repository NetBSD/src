#	$NetBSD: Makefile,v 1.2 2000/12/13 11:18:38 enami Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32 $S/arch/${MACHINE}/${MACHINE}

CPPFLAGS+=	-DVM86 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_12 -DCOMPAT_43
CPPFLAGS+=	-DEXEC_AOUT -DEXEC_ELF32 # for netbsd32_exec.c
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

MKMAN=	no

KMOD=	compat_netbsd32

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	netbsd32_compat_09.c netbsd32_compat_10.c netbsd32_compat_12.c \
	netbsd32_compat_13.c netbsd32_compat_14.c netbsd32_compat_43.c \
	netbsd32_netbsd.c netbsd32_syscalls.c netbsd32_sysent.c \
	netbsd32_ioctl.c

.include <bsd.kmod.mk>
