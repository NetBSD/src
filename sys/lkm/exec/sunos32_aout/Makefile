#	$NetBSD: Makefile,v 1.4.6.2 2004/09/18 14:54:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_NETBSD32

KMOD=	exec_sunos32_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos32_exec_aout.c

.include <bsd.kmod.mk>
