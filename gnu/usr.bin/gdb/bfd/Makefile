# $Id: Makefile,v 1.3 1994/09/19 23:51:14 deraadt Exp $

LIB=		bfd
NOPROFILE=
NOPIC=

CFLAGS+=	-I$(.CURDIR)/arch/$(MACHINE_ARCH) -I$(.CURDIR) \
		-I$(.CURDIR)/../include
.PATH:		$(.CURDIR)/arch/$(MACHINE_ARCH)

SRCS=		archive.c archures.c bfd.c cache.c coffgen.c core.c ctor.c \
		format.c init.c libbfd.c opncls.c reloc.c seclet.c section.c \
		syms.c targets.c ecoff.c elf.c srec.c aout32.c \
		stab-syms.c netbsd-core.c

.include "arch/$(MACHINE_ARCH)/Makefile.inc"

targets.o archures.o: $(.CURDIR)/$(.TARGET:S/.o$/.c/)
	$(CC) $(CFLAGS) $(VECTORS) -c -o $(.TARGET) $(.IMPSRC)


install:

.include <bsd.lib.mk>
