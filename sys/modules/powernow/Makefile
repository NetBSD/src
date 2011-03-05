# $NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:46 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	powernow
IOCONF=	powernow.ioconf
SRCS=	powernow.c

WARNS=	3

.include <bsd.kmodule.mk>
