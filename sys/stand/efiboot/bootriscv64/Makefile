# $NetBSD: Makefile,v 1.1 2021/05/02 15:22:27 jmcneill Exp $

PROG=		bootriscv64.efi
OBJFMT=		binary
NEWVERSWHAT=	"efiboot (riscv64)"
GNUEFIARCH=	riscv64
LIBGNUEFI_ARCH=	riscv64

EXTRA_SOURCES=	efibootriscv64.c

COPTS+=		-mcmodel=medany
AFLAGS+=	-x assembler-with-cpp
CFLAGS+=	-DEFIBOOT_ALIGN=0x200000
CFLAGS+=	-DEFIBOOT_MODULE_MACHINE=\"riscv\"

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
