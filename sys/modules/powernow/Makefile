# $NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:36 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	powernow
IOCONF=	powernow.ioconf
SRCS=	powernow.c

WARNS=	3

.include <bsd.kmodule.mk>
