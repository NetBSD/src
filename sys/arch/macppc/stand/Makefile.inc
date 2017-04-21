#	$NetBSD: Makefile.inc,v 1.3.206.1 2017/04/21 16:53:31 bouyer Exp $

BINDIR=		/usr/mdec

CFLAGS_UNWIND.gcc=	-fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS_UNWIND.clang=	-fno-unwind-tables

LINKFLAGS_UNWIND=	--no-ld-generated-unwind-info
