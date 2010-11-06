#	$NetBSD: Makefile,v 1.1.2.2 2010/11/06 08:08:46 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpidalb
IOCONF=	ACPIDALB.ioconf
SRCS=	dalb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
