#	$NetBSD: Makefile,v 1.4 2019/02/17 04:05:56 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

.include <bsd.kmodule.mk>
