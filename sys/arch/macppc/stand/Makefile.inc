#	$NetBSD: Makefile.inc,v 1.3.202.1 2017/04/26 02:53:04 pgoyette Exp $

BINDIR=		/usr/mdec

CFLAGS_UNWIND.gcc=	-fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS_UNWIND.clang=	-fno-unwind-tables

LINKFLAGS_UNWIND=	--no-ld-generated-unwind-info
