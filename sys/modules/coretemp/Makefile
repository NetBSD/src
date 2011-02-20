# $NetBSD: Makefile,v 1.1 2011/02/20 13:48:26 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	coretemp
IOCONF=	coretemp.ioconf
SRCS=	coretemp.c

WARNS=	4

.include <bsd.kmodule.mk>
