#	$NetBSD: Makefile,v 1.4 1997/10/17 20:51:25 gwr Exp $

LIB=	bfd

NOLINT=
NOMAN=
NOPROFILE=

TOP=	$(.CURDIR)/../..
dist=	$(TOP)/dist
srcdir= $(dist)/bfd

INCSDIR=/usr/include/bfd
INCS= ansidecl.h bfd.h libiberty.h

# CFLAGS=-g #XXX
CFLAGS+= -I$(.CURDIR)
CFLAGS+= -I$(dist)/bfd
CFLAGS+= -I$(dist)/opcodes
# ommitted -I$(dist)/libiberty on purpose
CFLAGS+= -I$(dist)/include

.PATH: $(dist)/bfd
.PATH: $(dist)/opcodes
.PATH: $(dist)/libiberty

.if exists(_${MACHINE_ARCH}.mk)
.include  "_${MACHINE_ARCH}.mk"
.endif

# From $(dist)/bfd: BFD_LIBS, BFD_BACKENDS, BFD_MACHINES, ...
# Have to ommit libbfd.c to avoid .c.so rule problem.
BFD_SRCS= \
	archive.c archures.c bfd.c cache.c coffgen.c corefile.c \
	format.c init.c opncls.c reloc.c \
	section.c syms.c targets.c hash.c linker.c \
	elf.c srec.c binary.c tekhex.c ihex.c stabs.c stab-syms.c \
	$(BFD_BACKENDS) $(BFD_MACHINES) netbsd-core.c

# From $(dist)/opcodes: BFD_MACHINES, dis*
OPCODE_SRCS= $(OPCODE_MACHINES) dis-buf.o disassemble.o

# From $(dist)/libiberty: REQUIRED_OFILES, HOST_OFILES
IBERTY_SRCS= argv.c basename.c choose-temp.c concat.c cplus-dem.c \
  fdmatch.c fnmatch.c getopt.c getopt1.c getruntime.c hex.c \
  floatformat.c objalloc.c obstack.c spaces.c strerror.c strsignal.c \
  xatexit.c xexit.c xmalloc.c xstrdup.c xstrerror.c \
  insque.c vasprintf.c

SRCS = $(BFD_SRCS) $(OPCODE_SRCS) $(IBERTY_SRCS)
OBJS = misc.o
.include <bsd.lib.mk>

CC_o=	$(CC) $(CFLAGS) $(CPPFLAGS)
CC_so=	$(CC) $(CFLAGS) $(CPPFLAGS) $(CPICFLAGS)

# Add $(ARCH_DEFS) when compiling these...
archures.o: $(dist)/bfd/archures.c
	$(CC_o)  $(ARCH_DEFS) -c $(.IMPSRC)
archures.so: $(dist)/bfd/archures.c
	$(CC_so) $(ARCH_DEFS) -c $(.IMPSRC) -o $@

# Add $(ARCH_DEFS) when compiling these...
disassemble.o: $(dist)/opcodes/disassemble.c
	$(CC_o)  $(ARCH_DEFS) -c $(.IMPSRC)
disassemble.so: $(dist)/opcodes/disassemble.c
	$(CC_so) $(ARCH_DEFS) -c $(.IMPSRC) -o $@

# Add $(TDEFAULTS) when compiling these...
targets.o: $(dist)/bfd/targets.c
	$(CC_o)  $(TDEFAULTS) -c $(.IMPSRC)
targets.so: $(dist)/bfd/targets.c
	$(CC_so) $(TDEFAULTS) -c $(.IMPSRC) -o $@

# Avoid the unwanted .c.a default rule
.NOPATH: libbfd.a libbfd.o libbfd.so
# XXX $#!%  Why do I STILL get this behaviour:
# building standard bfd library
# nm: .../bfd/libbfd.c: not object file or archive

misc.o: $(dist)/bfd/libbfd.c
	$(CC_o)  -c $(dist)/bfd/libbfd.c -o $@
misc.so: $(dist)/bfd/libbfd.c
	$(CC_so) -c $(dist)/bfd/libbfd.c -o $@

# Icky generated header files...
CLEANFILES+= elf32-target.h elf64-target.h
elf32-target.h : $(dist)/bfd/elfxx-target.h
	-rm -f $@
	sed -e s/NN/32/g < $(dist)/bfd/elfxx-target.h > $@
elf64-target.h : $(dist)/bfd/elfxx-target.h
	-rm -f $@
	sed -e s/NN/64/g < $(dist)/bfd/elfxx-target.h > $@

# Some dependencies, by hand.
argv.o : alloca-conf.h
${OBJS} : ansidecl.h bfd.h config.h libiberty.h
targets.o : targmatch.h

# Make sure we build elf32-target.h when needed.
elf32-gen.o   : elf32-target.h
elf32-i386.o  : elf32-target.h
elf32-m68k.o  : elf32-target.h
elf32-mips.o  : elf32-target.h
elf32-ppc.o   : elf32-target.h
elf32-sparc.o : elf32-target.h

# Make sure we build elf64-target.h when needed.
elf64-alpha.o : elf64-target.h
elf64-gen.o   : elf64-target.h
elf64-mips.o  : elf64-target.h
elf64-sparc.o : elf64-target.h


