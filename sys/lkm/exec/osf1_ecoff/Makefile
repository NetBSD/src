#	$NetBSD: Makefile,v 1.3 2001/12/12 11:49:07 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_osf1_ecoff

SRCS=	lkminit_exec.c
SRCS+=	osf1_exec_ecoff.c

.include <bsd.kmod.mk>
