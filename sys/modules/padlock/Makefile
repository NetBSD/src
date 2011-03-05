#	$NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:46 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

.include <bsd.kmodule.mk>
