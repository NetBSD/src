#	$NetBSD: Makefile,v 1.4 2002/05/30 18:53:21 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_osf1_ecoff

SRCS=	lkminit_exec.c
SRCS+=	osf1_exec_ecoff.c

.include <bsd.kmod.mk>
