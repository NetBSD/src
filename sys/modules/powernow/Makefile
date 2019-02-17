# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:56 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	powernow
IOCONF=	powernow.ioconf
SRCS=	powernow.c

WARNS=	3

.include <bsd.kmodule.mk>
