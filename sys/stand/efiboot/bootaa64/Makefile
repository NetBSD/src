# $NetBSD: Makefile,v 1.12 2022/04/02 11:16:07 skrll Exp $

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
CFLAGS+=	-DEFIBOOT_RUNTIME_ADDRESS=0x0000000200200000UL
CFLAGS+=	-DEFIBOOT_RUNTIME_SIZE=0xfe00000UL
CFLAGS+=	-DEFIBOOT_MODULE_MACHINE=\"evbarm\"
LDFLAGS+=	-EL

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
