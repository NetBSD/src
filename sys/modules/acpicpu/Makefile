# $NetBSD: Makefile,v 1.2 2010/08/08 16:58:42 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/acpi

KMOD=	acpicpu
SRCS=	acpi_cpu.c acpi_cpu_cstate.c acpi_cpu_pstate.c acpi_cpu_md.c

WARNS=	4

.include <bsd.kmodule.mk>
