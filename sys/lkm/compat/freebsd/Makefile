#	$NetBSD: Makefile,v 1.7 2000/12/08 20:15:14 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd

CPPFLAGS+=	-DVM86 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_FREEBSD
CPPFLAGS+=	-nostdinc
AFLAGS=         -x assembler-with-cpp -traditional-cpp -D_LOCORE \
                -I. -I${S}

MKMAN=	no

KMOD=	compat_freebsd

# common source files
SRCS=	lkminit_emul.c
SRCS+=	freebsd_exec.c freebsd_file.c freebsd_ioctl.c freebsd_ipc.c \
	freebsd_misc.c freebsd_ptrace.c freebsd_syscalls.c freebsd_sysent.c

# MD source files
.if ${MACHINE} == "i386"
.PATH: $S/arch/i386/i386
SRCS+=	freebsd_machdep.c freebsd_sigcode.s
.endif

.include <bsd.kmod.mk>
