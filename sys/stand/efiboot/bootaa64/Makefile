# $NetBSD: Makefile,v 1.11 2021/10/06 10:13:19 jmcneill Exp $

PROG=		bootaa64.efi
OBJFMT=		binary
NEWVERSWHAT=	"efiboot (arm64)"
GNUEFIARCH=	aarch64
LIBGNUEFI_ARCH=	aarch64
OBJCOPY_FORMAT=	-O binary

EFIBOOT_ACPI=	yes
EFIBOOT_FDT=	yes

EXTRA_SOURCES=	efibootaa64.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mgeneral-regs-only -mlittle-endian -fno-jump-tables
AFLAGS+=	-mlittle-endian
CFLAGS+=	-DEFIBOOT_ALIGN=0x200000
CFLAGS+=	-DEFIBOOT_RUNTIME_ADDRESS=0xffff800000000000L
CFLAGS+=	-DEFIBOOT_RUNTIME_SIZE=0x40000000UL
CFLAGS+=	-DEFIBOOT_MODULE_MACHINE=\"evbarm\"
LDFLAGS+=	-EL

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
