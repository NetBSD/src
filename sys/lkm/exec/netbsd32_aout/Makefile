#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:48 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_NETBSD32

KMOD=	exec_netbsd32_aout

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_aout.c

.include <bsd.kmod.mk>
