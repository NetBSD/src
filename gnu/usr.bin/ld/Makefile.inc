#	$NetBSD: Makefile.inc,v 1.3 1998/12/17 15:17:27 pk Exp $

SHLIBPATH = $(.CURDIR)/../../../../libexec/ld.aout_so

CPPFLAGS += -I$(SHLIBPATH) -I$(SHLIBPATH)/arch/$(MACHINE_ARCH) -I$(.CURDIR)
.PATH:	      $(SHLIBPATH)   $(SHLIBPATH)/arch/$(MACHINE_ARCH)

CFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes
