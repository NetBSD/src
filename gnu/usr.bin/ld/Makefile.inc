#	$NetBSD: Makefile.inc,v 1.4 2001/07/25 11:58:04 aymeric Exp $

SHLIBPATH = $(.CURDIR)/../../../../libexec/ld.aout_so

TARGET_MACHINE_ARCH ?= $(MACHINE_ARCH)

.if $(TARGET_MACHINE_ARCH) != $(MACHINE_ARCH)
CPPFLAGS += -DCROSS_LINKER -DXHOST="$(MACHINE_ARCH)"
.endif

CPPFLAGS += -I$(SHLIBPATH) -I$(SHLIBPATH)/arch/$(TARGET_MACHINE_ARCH) -I$(.CURDIR)
.PATH:	      $(SHLIBPATH)   $(SHLIBPATH)/arch/$(TARGET_MACHINE_ARCH)

CFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes
