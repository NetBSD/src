#	$NetBSD: Makefile,v 1.1 2002/11/16 08:10:50 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

real-vnd.c:
	ln -s ${S}/dev/vnd.c real-vnd.c

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c
CLEANFILES+=	real-vnd.c

.include <bsd.kmod.mk>
