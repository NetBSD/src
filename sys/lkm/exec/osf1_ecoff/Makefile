#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:12 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/osf1

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

NOMAN=	# defined

KMOD=	exec_osf1_ecoff

SRCS=	lkminit_exec.c
SRCS+=	osf1_exec_ecoff.c

.include <bsd.kmod.mk>
