#	$NetBSD: Makefile,v 1.5 2001/12/12 01:50:09 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix $S/arch/${MACHINE}/${MACHINE}

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_ULTRIX
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

NOMAN=	# defined

KMOD=	compat_ultrix

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	ultrix_fs.c ultrix_ioctl.c ultrix_misc.c ultrix_pathname.c \
	ultrix_syscalls.c ultrix_sysent.c

.if ${MACHINE_CPU} == "mips"
CPPFLAGS+=	-DMIPS1 -DMIPS3
.endif

.include <bsd.kmod.mk>
