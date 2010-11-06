#	$NetBSD: Makefile,v 1.1.2.2 2010/11/06 08:08:47 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	asus
IOCONF=	ASUS.ioconf
SRCS=	asus_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
