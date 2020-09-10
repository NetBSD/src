# $NetBSD: Makefile,v 1.10 2020/09/10 14:30:14 jakllsch Exp $

PROG=		bootaa64.efi
OBJFMT=		binary
NEWVERSWHAT=	"efiboot (arm64)"
GNUEFIARCH=	aarch64
LIBGNUEFI_ARCH=	aarch64

EXTRA_SOURCES=	efibootaa64.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mgeneral-regs-only -mlittle-endian -fno-jump-tables
AFLAGS+=	-mlittle-endian
CFLAGS+=	-DEFIBOOT_ALIGN=0x200000
CFLAGS+=	-DEFIBOOT_RUNTIME_ADDRESS=0xffff800000000000L
CFLAGS+=	-DEFIBOOT_RUNTIME_SIZE=0x40000000UL
CFLAGS+=	-DEFIBOOT_ACPI
CFLAGS+=	-DEFIBOOT_MODULE_MACHINE=\"evbarm\"
LDFLAGS+=	-EL

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
