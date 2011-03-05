#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:45 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

.include <bsd.kmodule.mk>
