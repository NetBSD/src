# $NetBSD: Makefile,v 1.5 2018/10/28 10:17:47 jmcneill Exp $

PROG=		bootaa64.efi
OBJFMT=		binary
NEWVERSWHAT=	"EFI Boot (aarch64)"

EXTRA_SOURCES=	efibootaa64.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mgeneral-regs-only -fno-jump-tables
CFLAGS+=	-DEFIBOOT_ALIGN=0x200000
CFLAGS+=	-DEFIBOOT_RUNTIME_ADDRESS=0xffff800000000000L
CFLAGS+=	-DEFIBOOT_RUNTIME_SIZE=0x40000000UL
CFLAGS+=	-DEFIBOOT_ACPI

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${MACHINE}/installation/misc
