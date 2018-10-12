# $NetBSD: Makefile,v 1.3 2018/10/12 22:08:04 jmcneill Exp $

PROG=		bootaa64.efi
OBJFMT=		binary
NEWVERSWHAT=	"EFI Boot (aarch64)"

EXTRA_SOURCES=	efibootaa64.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mgeneral-regs-only -fno-jump-tables
CFLAGS+=	-DEFIBOOT_ALIGN=0x200000
#CFLAGS+=	-DEFIBOOT_ACPI

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${MACHINE}/installation/misc
