# $NetBSD: Makefile,v 1.1 2010/07/18 09:37:50 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/acpi

KMOD=	acpicpu
SRCS=	acpi_cpu.c acpi_cpu_cstate.c acpi_cpu_md.c

WARNS=	4

.include <bsd.kmodule.mk>
