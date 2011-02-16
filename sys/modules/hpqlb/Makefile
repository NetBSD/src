#	$NetBSD: Makefile,v 1.2 2011/02/16 08:08:14 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpqlb
IOCONF=	hpqlb.ioconf
SRCS=	hpqlb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
