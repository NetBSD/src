# $NetBSD: Makefile,v 1.1 2011/03/04 05:02:47 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	odcm
IOCONF=	odcm.ioconf
SRCS=	odcm.c

WARNS=	4

.include <bsd.kmodule.mk>
