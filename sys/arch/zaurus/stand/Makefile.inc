#	$NetBSD: Makefile.inc,v 1.10 2023/06/03 21:26:28 lukem Exp $

BINDIR=		/usr/mdec
KLINK_MACHINE=	zaurus

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
