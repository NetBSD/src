# $NetBSD: Makefile,v 1.1 2011/02/24 11:22:34 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	powernow
IOCONF=	powernow.ioconf
SRCS=	powernow.c

WARNS=	3

.include <bsd.kmodule.mk>
