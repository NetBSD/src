#	$NetBSD: Makefile,v 1.1 2015/05/10 07:41:16 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysv

CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50
CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS=	compat_sysv_mod.c
SRCS+=	sysv_msg_14.c sysv_sem_14.c sysv_shm_14.c
SRCS+=	sysv_msg_50.c sysv_sem_50.c sysv_shm_50.c sysv_ipc_50.c

# really, all machines where sizeof(int) != sizeof(long) (LP64)
.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "sparc64" \
  && ${MACHINE_ARCH} != "x86_64")
SRCS+=  kern_ipc_10.c
.endif

.include <bsd.kmodule.mk>
