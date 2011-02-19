#	$NetBSD: Makefile,v 1.1 2011/02/19 14:25:30 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

.include <bsd.kmodule.mk>
