# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:26 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic
.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/pci

KMOD=	hpet
IOCONF=	hpet.ioconf
SRCS=	hpet.c hpet_acpi.c amdpcib_hpet.c ichlpcib_hpet.c

.include <bsd.kmodule.mk>
