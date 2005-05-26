#	$NetBSD: Makefile,v 1.3 2005/05/26 18:33:36 he Exp $

.include "../Makefile.inc"

.PATH: $S/net

CPPFLAGS+=-DDO_BSD_COMPRESS=1
MKMAN=	no
KMOD=	bsdcomp
SRCS=	lkminit_bsdcomp.c bsd-comp.c

.if ${MACHINE_CPU} == "sh5"
COPTS.bsd-comp.c=	-O1
.endif

.include <bsd.kmod.mk>
