#	$NetBSD: Makefile,v 1.1.6.1 2002/01/10 20:01:07 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_osf1_ecoff

SRCS=	lkminit_exec.c
SRCS+=	osf1_exec_ecoff.c

.include <bsd.kmod.mk>
