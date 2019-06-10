#	$NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:25 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/flash

KMOD=	flash
IOCONF=	flash.ioconf
SRCS=	flash.c flash_io.c

WARNS=	3

.include <bsd.kmodule.mk>
