#	$NetBSD: Makefile,v 1.1.2.2 2001/02/11 19:16:55 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

MKMAN=	no

KMOD=	exec_sunos32_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos32_exec_aout.c

.include <bsd.kmod.mk>
