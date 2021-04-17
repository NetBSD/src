#	$NetBSD: Makefile.inc,v 1.7.4.1 2021/04/17 17:26:17 thorpej Exp $

BINDIR=		/usr/mdec
KLINK_MACHINE=	zaurus

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}
