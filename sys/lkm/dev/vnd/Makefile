#	$NetBSD: Makefile,v 1.1.8.3 2004/09/21 13:36:24 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c

BUILDSYMLINKS+=${S}/dev/vnd.c real-vnd.c

.include <bsd.kmod.mk>
