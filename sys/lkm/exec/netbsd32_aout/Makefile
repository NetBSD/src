#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:11 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT -DCOMPAT_NETBSD32

NOMAN=	# defined

KMOD=	exec_netbsd32_aout

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_aout.c

.include <bsd.kmod.mk>
