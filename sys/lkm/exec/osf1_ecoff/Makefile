#	$NetBSD: Makefile,v 1.1 2000/12/08 23:05:44 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

MKMAN=	no

KMOD=	exec_osf1_ecoff

SRCS=	lkminit_exec.c
SRCS+=	osf1_exec_ecoff.c

.include <bsd.kmod.mk>
