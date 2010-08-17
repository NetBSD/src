# $NetBSD: Makefile,v 1.3.2.2 2010/08/17 06:47:40 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/acpi

KMOD=	acpicpu
SRCS=	acpi_cpu.c \
	acpi_cpu_md.c \
	acpi_cpu_cstate.c \
	acpi_cpu_pstate.c \
	acpi_cpu_tstate.c

WARNS=	4

.include <bsd.kmodule.mk>
