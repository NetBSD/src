#	$NetBSD: Makefile,v 1.1.2.2 2002/12/11 06:46:28 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

real-vnd.c:
	ln -s ${S}/dev/vnd.c real-vnd.c

KMOD=	vnd

SRCS=   lkminit_vnd.c real-vnd.c
CLEANFILES+=	real-vnd.c

.include <bsd.kmod.mk>
