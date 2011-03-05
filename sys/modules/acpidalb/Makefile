#	$NetBSD: Makefile,v 1.2.2.2 2011/03/05 20:55:35 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpidalb
IOCONF=	acpidalb.ioconf
SRCS=	dalb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
