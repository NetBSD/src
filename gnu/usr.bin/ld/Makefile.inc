#	$NetBSD: Makefile.inc,v 1.1 1997/04/16 16:49:29 christos Exp $

CFLAGS += -I$(.CURDIR)/../common -I$(.CURDIR)/../arch/$(MACHINE_ARCH)

.PATH: $(.CURDIR)/../common $(.CURDIR)/../arch/$(MACHINE_ARCH)
