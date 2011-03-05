# $NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:39 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	coretemp
IOCONF=	coretemp.ioconf
SRCS=	coretemp.c

WARNS=	4

.include <bsd.kmodule.mk>
