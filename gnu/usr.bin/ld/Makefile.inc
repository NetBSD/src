#	$NetBSD: Makefile.inc,v 1.2 1997/10/25 10:12:41 lukem Exp $

CPPFLAGS += -I$(.CURDIR)/../common -I$(.CURDIR)/../arch/$(MACHINE_ARCH)

.PATH: $(.CURDIR)/../common $(.CURDIR)/../arch/$(MACHINE_ARCH)
