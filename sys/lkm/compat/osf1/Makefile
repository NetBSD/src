#	$NetBSD: Makefile,v 1.3.4.3 2002/04/17 00:06:21 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1 $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_OSF1
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

KMOD=	compat_osf1

# common source files
SRCS=	lkminit_emul.c
SRCS+=	osf1_cvt.c osf1_errno.c osf1_signo.c osf1_descrip.c \
	osf1_exec.c osf1_file.c osf1_generic.c osf1_ioctl.c osf1_misc.c	\
	osf1_mmap.c osf1_mount.c osf1_prot.c osf1_resource.c osf1_signal.c \
	osf1_socket.c osf1_syscalls.c osf1_sysent.c osf1_sysv_ipc.c \
	osf1_time.c

# MD source files
.if ${MACHINE_ARCH} == "alpha"
SRCS+=	osf1_syscall.c
SRCS+=	osf1_sigcode.s
.endif

.include <bsd.kmod.mk>
