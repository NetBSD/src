# $NetBSD: Makefile,v 1.2.2.3 2010/10/09 03:32:35 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/acpi

KMOD=	acpicpu
SRCS=	acpi_cpu.c \
	acpi_cpu_md.c \
	acpi_cpu_cstate.c \
	acpi_cpu_pstate.c \
	acpi_cpu_tstate.c

WARNS=	 4
CFLAGS+= -DDIAGNOSTIC

.include <bsd.kmodule.mk>
