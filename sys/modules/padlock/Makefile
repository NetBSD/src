#	$NetBSD: Makefile,v 1.3 2011/07/04 16:20:46 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

.include <bsd.kmodule.mk>
