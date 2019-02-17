#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:50 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpqlb
IOCONF=	hpqlb.ioconf
SRCS=	hpqlb_acpi.c

.include <bsd.kmodule.mk>
