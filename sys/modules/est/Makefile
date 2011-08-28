# $NetBSD: Makefile,v 1.2 2011/08/28 15:48:19 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	est
IOCONF=	est.ioconf
SRCS=	est.c intel_busclock.c

.include <bsd.kmodule.mk>
