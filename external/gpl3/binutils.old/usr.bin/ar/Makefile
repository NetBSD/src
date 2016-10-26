#	$NetBSD: Makefile,v 1.4 2016/10/26 17:09:38 christos Exp $

PROG=		ar

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
