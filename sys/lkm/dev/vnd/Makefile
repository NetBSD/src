#	$NetBSD: Makefile,v 1.1.8.1 2004/08/03 10:53:58 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c

BUILDSYMLINKS+=${S}/dev/vnd.c real-vnd.c

.include <bsd.kmod.mk>
