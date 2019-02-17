#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:49 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/flash

KMOD=	flash
IOCONF=	flash.ioconf
SRCS=	flash.c flash_io.c

WARNS=	3

.include <bsd.kmodule.mk>
