#	$NetBSD: Makefile,v 1.2.2.2 2001/01/08 14:57:05 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2 $S/arch/${MACHINE}/${MACHINE}

CPPFLAGS+= -DCOMPAT_SVR4 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DSVR4_COMPAT_SOLARIS2
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

MKMAN=	no
KMOD=	compat_ibcs2

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	ibcs2_errno.c ibcs2_exec.c ibcs2_fcntl.c ibcs2_ioctl.c ibcs2_ipc.c \
	ibcs2_misc.c ibcs2_signal.c ibcs2_socksys.c ibcs2_stat.c	\
	ibcs2_syscalls.c ibcs2_sysent.c

# MD source
.if ${MACHINE} == "i386"
.PATH: $S/arch/i386/i386
SRCS+=	ibcs2_sigcode.s ibcs2_machdep.c ibcs2_syscall.c
.endif

.include <bsd.kmod.mk>
