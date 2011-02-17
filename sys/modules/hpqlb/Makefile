#	$NetBSD: Makefile,v 1.1.6.1 2011/02/17 12:00:48 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpqlb
IOCONF=	hpqlb.ioconf
SRCS=	hpqlb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
