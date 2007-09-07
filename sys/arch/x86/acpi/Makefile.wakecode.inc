# $NetBSD: Makefile.wakecode.inc,v 1.1.2.1 2007/09/07 21:01:06 jmcneill Exp $
# FreeBSD: src/sys/i386/acpica/Makefile,v 1.3 2002/05/01 21:54:07 peter Exp

.include <bsd.sys.mk>		# for HOST_SH

all: acpi_wakecode.h

acpi_wakecode.h: acpi_wakecode.S genwakecode.sh
	${CC} -c acpi_wakecode.S
	${LD} -M -Map acpi_wakecode.bin.map -z defs -nostdlib -o acpi_wakecode.bin --oformat binary -e wakeup_16 -Ttext 0 acpi_wakecode.o
	${HOST_SH} ${.CURDIR}/genwakecode.sh > acpi_wakecode.h
	rm -f acpi_wakecode.bin acpi_wakecode.o acpi_wakecode.bin.map
