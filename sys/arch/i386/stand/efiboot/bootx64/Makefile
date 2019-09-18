#	$NetBSD: Makefile,v 1.1.12.1 2019/09/18 17:30:05 martin Exp $

PROG=		bootx64.efi
OBJFMT=		pei-x86-64
NEWVERSWHAT=	"EFI Boot (x64)"

EXTRA_SOURCES=	efibootx64.c startprog64.S multiboot64.S

COPTS+=		-mno-red-zone
CPPFLAGS+=	-DEFI_FUNCTION_WRAPPER

.include "${.CURDIR}/../Makefile.efiboot"
