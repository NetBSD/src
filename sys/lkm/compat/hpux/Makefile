#	$NetBSD: Makefile,v 1.2 2000/12/13 11:18:38 enami Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/hpux $S/arch/${MACHINE}/${MACHINE}

CPPFLAGS+=	-DVM86 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_HPUX
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

MKMAN=	no

KMOD=	compat_hpux

# common source files
SRCS=	lkminit_emul.c
SRCS+=	hpux_compat.c hpux_errno.c hpux_exec.c hpux_file.c hpux_ipc.c \
	hpux_net.c hpux_sig.c hpux_syscalls.c hpux_sysent.c hpux_tty.c
SRCS+=	hpux_machdep.c

# MD source files
.if ${MACHINE} == "hp300"
.endif

.include <bsd.kmod.mk>
