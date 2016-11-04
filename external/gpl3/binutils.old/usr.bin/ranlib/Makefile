#	$NetBSD: Makefile,v 1.2.2.1 2016/11/04 14:45:10 pgoyette Exp $

PROG=		ranlib

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
