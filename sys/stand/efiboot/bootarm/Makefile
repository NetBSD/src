# $NetBSD: Makefile,v 1.2 2019/09/23 13:42:37 christos Exp $

PROG=		bootarm.efi
OBJFMT=		binary
NEWVERSWHAT=	"EFI Boot (arm)"
GNUEFIARCH=	arm
LIBGNUEFI_ARCH=	arm

EXTRA_SOURCES=	efibootarm.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mno-unaligned-access -ffreestanding -fno-unwind-tables
CFLAGS+=	-DEFIBOOT_ALIGN=0x1000000
LDFLAGS+=	-N

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
