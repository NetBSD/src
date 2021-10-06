# $NetBSD: Makefile,v 1.8 2021/10/06 10:15:20 jmcneill Exp $

PROG=		bootarm.efi
OBJFMT=		binary
NEWVERSWHAT=	"efiboot (arm)"
GNUEFIARCH=	arm
LIBGNUEFI_ARCH=	arm
OBJCOPY_FORMAT=	-O binary

EFIBOOT_FDT=	yes

EXTRA_SOURCES=	efibootarm.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mfloat-abi=soft -mno-unaligned-access -mlittle-endian
COPTS+=		-ffreestanding -fno-unwind-tables
AFLAGS+=	-mlittle-endian
CFLAGS+=	-DEFIBOOT_ALIGN=0x1000000
CFLAGS+=	-DEFIBOOT_MODULE_MACHINE=\"evbarm\"
LDFLAGS+=	-N -EL

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
