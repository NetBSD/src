#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:42 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c

BUILDSYMLINKS+=${S}/dev/vnd.c real-vnd.c

.include <bsd.kmod.mk>
