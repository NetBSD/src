#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:12 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_osf1_ecoff

SRCS=	lkminit_exec.c
SRCS+=	osf1_exec_ecoff.c

.include <bsd.kmod.mk>
