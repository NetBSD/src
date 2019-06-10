# $NetBSD: Makefile,v 1.6.58.1 2019/06/10 22:09:07 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/acpi

KMOD=	acpicpu
IOCONF=	acpicpu.ioconf
SRCS=	acpi_cpu.c \
	acpi_cpu_md.c \
	acpi_cpu_cstate.c \
	acpi_cpu_pstate.c \
	acpi_cpu_tstate.c

.include <bsd.kmodule.mk>
