#	$NetBSD: Makefile,v 1.1.2.4 2005/11/10 14:10:25 skrll Exp $

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
