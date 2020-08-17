#	$NetBSD: Makefile,v 1.1.26.2 2020/08/17 10:30:22 martin Exp $

PROG=		bootx64.efi
OBJFMT=		pei-x86-64
NEWVERSWHAT=	"EFI Boot (x64)"

EXTRA_SOURCES=	efibootx64.c startprog64.S multiboot64.S

COPTS+=		-mno-red-zone
CPPFLAGS+=	-DEFI_FUNCTION_WRAPPER

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

.include "${.CURDIR}/../Makefile.efiboot"
