#	$NetBSD: Makefile,v 1.1.6.2 2019/06/10 22:09:41 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=

.PATH:	${S}/dev/tprof

KMOD=	tprof_x86
SRCS=	tprof_x86.c tprof_x86_amd.c tprof_x86_intel.c

.include <bsd.kmodule.mk>
