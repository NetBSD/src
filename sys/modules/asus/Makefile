#	$NetBSD: Makefile,v 1.1.4.1 2011/06/06 09:09:46 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	asus
IOCONF=	asus.ioconf
SRCS=	asus_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
