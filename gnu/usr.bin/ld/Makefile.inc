#	$NetBSD: Makefile.inc,v 1.5 2002/09/19 03:09:41 lukem Exp $

.include <bsd.own.mk>

SHLIBPATH = ${NETBSDSRCDIR}/libexec/ld.aout_so

TARGET_MACHINE_ARCH ?= $(MACHINE_ARCH)

.if $(TARGET_MACHINE_ARCH) != $(MACHINE_ARCH)
CPPFLAGS += -DCROSS_LINKER -DXHOST="$(MACHINE_ARCH)"
.endif

CPPFLAGS += -I$(SHLIBPATH) -I$(SHLIBPATH)/arch/$(TARGET_MACHINE_ARCH) -I$(.CURDIR)
.PATH:	      $(SHLIBPATH)   $(SHLIBPATH)/arch/$(TARGET_MACHINE_ARCH)

CFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes
