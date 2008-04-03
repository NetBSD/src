#	$NetBSD: Makefile,v 1.5.30.1 2008/04/03 12:43:06 mjf Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.include <bsd.kmod.mk>
