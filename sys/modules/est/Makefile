# $NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:23 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	est
IOCONF=	est.ioconf
SRCS=	est.c intel_busclock.c

WARNS=	3

.include <bsd.kmodule.mk>
