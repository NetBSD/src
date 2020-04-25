#	$NetBSD: Makefile.inc,v 1.5.28.1 2020/04/25 11:23:58 bouyer Exp $

BINDIR=		/usr/mdec
KLINK_MACHINE=	zaurus

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

COPTS+=	-fcommon
