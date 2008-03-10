#	$NetBSD: Makefile,v 1.6 2008/03/10 01:22:05 dholland Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.include <bsd.kmod.mk>
