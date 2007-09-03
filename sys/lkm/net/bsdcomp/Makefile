#	$NetBSD: Makefile,v 1.4.36.1 2007/09/03 07:05:06 wrstuden Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
MKMAN=	no
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.include <bsd.kmod.mk>
