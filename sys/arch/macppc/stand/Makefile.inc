#	$NetBSD: Makefile.inc,v 1.9 2023/06/03 21:26:28 lukem Exp $

BINDIR=		/usr/mdec

CFLAGS_UNWIND.gcc=	-fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS_UNWIND.clang=	-fno-unwind-tables

LINKFLAGS_UNWIND=	--no-ld-generated-unwind-info

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS+=		${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
