#	$NetBSD: Makefile,v 1.2 2019/09/13 02:19:46 manu Exp $

PROG=		bootx64.efi
OBJFMT=		pei-x86-64
NEWVERSWHAT=	"EFI Boot (x64)"

EXTRA_SOURCES=	efibootx64.c startprog64.S multiboot64.S

COPTS+=		-mno-red-zone
CPPFLAGS+=	-DEFI_FUNCTION_WRAPPER

.include "${.CURDIR}/../Makefile.efiboot"
