#	$NetBSD: Makefile,v 1.24.16.1 2002/01/10 19:50:58 thorpej Exp $

LIB=	compat
NOPIC=	# defined

CPPFLAGS=	${COMPATCPPFLAGS}

.PATH.c: ${COMPATDIR}

# Common compatibility code, used by all emulations
SRCS=	compat_exec.c compat_util.c

# Compatibility code for 4.3BSD
SRCS+=	kern_exit_43.c kern_info_43.c kern_resource_43.c kern_sig_43.c \
	tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c

# Compatibility code for NetBSD 0.9
SRCS+=	kern_info_09.c

# Compatibility code for NetBSD 1.2
SRCS+=	kern_xxx_12.c vfs_syscalls_12.c vm_12.c

# Compatibility code for NetBSD 1.3
SRCS+=	kern_sig_13.c

# Compatibility code for NetBSD 1.4
SRCS+=	sysv_ipc_14.c sysv_msg_14.c sysv_shm_14.c sysv_sem_14.c

# really, all machines where sizeof(int) != sizeof(long) (LP64)
.if (${MACHINE_ARCH} != "alpha")
SRCS+=	kern_ipc_10.c 
.endif

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
