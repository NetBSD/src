#	$NetBSD: Makefile,v 1.2 2003/09/04 07:18:01 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c

LNFILES+=${S}/dev/vnd.c real-vnd.c

.include <bsd.kmod.mk>
