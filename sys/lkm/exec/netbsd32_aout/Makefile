#	$NetBSD: Makefile,v 1.1.2.2 2001/01/08 14:57:22 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT -DCOMPAT_NETBSD32

MKMAN=	no

KMOD=	exec_netbsd32_aout

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_aout.c

.include <bsd.kmod.mk>
