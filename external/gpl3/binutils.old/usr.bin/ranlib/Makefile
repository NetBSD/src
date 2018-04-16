#	$NetBSD: Makefile,v 1.5.8.1 2018/04/16 01:59:41 pgoyette Exp $

PROG=		ranlib

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
