#	$NetBSD: Makefile,v 1.1.1.2 2016/10/26 17:03:45 christos Exp $

PROG=		ranlib

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
