#	$NetBSD: Makefile,v 1.5 2007/04/08 09:36:34 scw Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
MKMAN=	no
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.include <bsd.kmod.mk>
