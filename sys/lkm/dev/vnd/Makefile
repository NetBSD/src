#	$NetBSD: Makefile,v 1.4 2005/12/11 12:24:48 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c

BUILDSYMLINKS+=${S}/dev/vnd.c real-vnd.c

.include <bsd.kmod.mk>
