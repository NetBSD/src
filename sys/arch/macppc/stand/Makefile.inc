#	$NetBSD: Makefile.inc,v 1.5 2020/04/21 21:39:07 joerg Exp $

BINDIR=		/usr/mdec

CFLAGS_UNWIND.gcc=	-fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS_UNWIND.clang=	-fno-unwind-tables

LINKFLAGS_UNWIND=	--no-ld-generated-unwind-info

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

COPTS+=	-fcommon
