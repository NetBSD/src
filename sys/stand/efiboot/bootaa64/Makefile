# $NetBSD: Makefile,v 1.2 2018/08/27 22:40:51 jmcneill Exp $

PROG=		bootaa64.efi
OBJFMT=		binary
NEWVERSWHAT=	"EFI Boot (aarch64)"

EXTRA_SOURCES=	efibootaa64.c
EXTRA_SOURCES+=	cache.S

COPTS+=		-mgeneral-regs-only -fno-jump-tables
CFLAGS+=	-DEFIBOOT_ALIGN=0x200000

.include "${.CURDIR}/../Makefile.efiboot"

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
		${RELEASEDIR}/${MACHINE}/installation/misc
