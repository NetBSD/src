# $NetBSD: Makefile,v 1.1 2011/06/15 03:22:40 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	amdtemp
IOCONF=	amdtemp.ioconf
SRCS=	amdtemp.c

WARNS=	4

.include <bsd.kmodule.mk>
