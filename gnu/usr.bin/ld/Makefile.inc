#	$NetBSD: Makefile.inc,v 1.1.2.1 1997/11/08 06:54:10 lukem Exp $

CPPFLAGS += -I$(.CURDIR)/../common -I$(.CURDIR)/../arch/$(MACHINE_ARCH)

.PATH: $(.CURDIR)/../common $(.CURDIR)/../arch/$(MACHINE_ARCH)
