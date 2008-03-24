#	$NetBSD: Makefile,v 1.5.26.1 2008/03/24 07:16:23 keiichi Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.include <bsd.kmod.mk>
