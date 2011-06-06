# $NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:47 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	coretemp
IOCONF=	coretemp.ioconf
SRCS=	coretemp.c

WARNS=	4

.include <bsd.kmodule.mk>
