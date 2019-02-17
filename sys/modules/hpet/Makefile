# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:50 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic
.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/pci

KMOD=	hpet
IOCONF=	hpet.ioconf
SRCS=	hpet.c hpet_acpi.c amdpcib_hpet.c ichlpcib_hpet.c

.include <bsd.kmodule.mk>
