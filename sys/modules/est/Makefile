# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:48 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	est
IOCONF=	est.ioconf
SRCS=	est.c intel_busclock.c

WARNS=	3

.include <bsd.kmodule.mk>
