#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:11 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT -DCOMPAT_NETBSD32

KMOD=	exec_netbsd32_aout

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_aout.c

.include <bsd.kmod.mk>
