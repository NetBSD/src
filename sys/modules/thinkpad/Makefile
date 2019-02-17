# $NetBSD: Makefile,v 1.4 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	thinkpad
IOCONF=	thinkpad.ioconf
SRCS=	thinkpad_acpi.c

.include <bsd.kmodule.mk>
