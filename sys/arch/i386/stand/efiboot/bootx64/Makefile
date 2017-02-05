#	$NetBSD: Makefile,v 1.1.2.2 2017/02/05 13:40:13 skrll Exp $

PROG=		bootx64.efi
OBJFMT=		pei-x86-64
NEWVERSWHAT=	"EFI Boot (x64)"

EXTRA_SOURCES=	efibootx64.c startprog64.S

COPTS+=		-mno-red-zone
CPPFLAGS+=	-DEFI_FUNCTION_WRAPPER

.include "${.CURDIR}/../Makefile.efiboot"
