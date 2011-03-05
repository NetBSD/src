# $NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:43 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	coretemp
IOCONF=	coretemp.ioconf
SRCS=	coretemp.c

WARNS=	4

.include <bsd.kmodule.mk>
