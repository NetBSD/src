#	$NetBSD: Makefile,v 1.1.8.2 2004/09/18 14:54:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c

BUILDSYMLINKS+=${S}/dev/vnd.c real-vnd.c

.include <bsd.kmod.mk>
