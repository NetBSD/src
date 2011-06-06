# $NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:47 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	est
IOCONF=	est.ioconf
SRCS=	est.c intel_busclock.c

WARNS=	3

.include <bsd.kmodule.mk>
