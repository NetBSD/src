# $NetBSD: Makefile.wakecode.inc,v 1.1.10.1 2007/12/26 21:38:47 ad Exp $
# FreeBSD: src/sys/i386/acpica/Makefile,v 1.3 2002/05/01 21:54:07 peter Exp

ACPI_WAKECODE_S=	${S}/arch/${MACHINE}/acpi/acpi_wakecode.S
ACPI_GENWAKECODE=	${S}/arch/x86/acpi/genwakecode.sh

acpi_wakecode.h: ${ACPI_WAKECODE_S} ${ACPI_GENWAKECODE}
	${_MKTARGET_CREATE}
	${CC} -I${.CURDIR} -c ${ACPI_WAKECODE_S}
	${LD} -M -Map acpi_wakecode.bin.map -z defs -nostdlib \
	    --oformat binary -e wakeup_16 -Ttext 0 \
	    -o acpi_wakecode.bin acpi_wakecode.o
	${HOST_SH} ${ACPI_GENWAKECODE} > acpi_wakecode.h
	rm -f acpi_wakecode.bin acpi_wakecode.o acpi_wakecode.bin.map

acpi_wakeup.o acpi_wakeup.d: acpi_wakecode.h
