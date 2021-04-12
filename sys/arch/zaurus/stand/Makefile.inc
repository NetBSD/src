#	$NetBSD: Makefile.inc,v 1.8 2021/04/12 03:55:41 mrg Exp $

BINDIR=		/usr/mdec
KLINK_MACHINE=	zaurus

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}
