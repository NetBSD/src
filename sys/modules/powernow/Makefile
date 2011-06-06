# $NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:50 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	powernow
IOCONF=	powernow.ioconf
SRCS=	powernow.c

WARNS=	3

.include <bsd.kmodule.mk>
