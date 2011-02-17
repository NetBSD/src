#	$NetBSD: Makefile,v 1.1.6.1 2011/02/17 12:00:46 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpidalb
IOCONF=	acpidalb.ioconf
SRCS=	dalb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
