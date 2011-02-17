# $NetBSD: Makefile,v 1.2.4.1 2011/02/17 12:00:48 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	thinkpad
IOCONF=	thinkpad.ioconf
SRCS=	thinkpad_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
