#	$NetBSD: Makefile,v 1.21 1998/12/28 20:38:50 tv Exp $

LIB=		bfd

NOLINT=		nolint (GNU source)
NOMAN=
NOPROFILE=

SRCS=		archive.c archures.c bfd.c cache.c coffgen.c corefile.c \
		format.c init.c lib_bfd.c opncls.c reloc.c section.c syms.c \
		targets.c hash.c linker.c elf.c srec.c binary.c tekhex.c \
		ihex.c stabs.c stab-syms.c netbsd-core.c \
		dis-buf.c disassemble.c \
		argv.c basename.c choose-temp.c concat.c cplus-dem.c \
		fdmatch.c fnmatch.c getopt.c getopt1.c getruntime.c hex.c \
		floatformat.c objalloc.c obstack.c pexecute.c spaces.c \
		strerror.c strsignal.c xatexit.c xexit.c xmalloc.c xstrdup.c \
		xstrerror.c insque.c \
		${BFD_ARCH_SRCS}
CPPFLAGS+= 	-I. -I${.CURDIR} \
		-I${DIST}/bfd -I${DIST}/opcodes -I${DIST}/include \
		${BFD_ARCH_DEFS} -DHAVE_CONFIG_H

DIST=		${.CURDIR}/../../dist

INCSDIR=	/usr/include/bfd
INCS=		ansidecl.h bfd.h libiberty.h

DPSRCS+=	bfd.h elf32-target.h elf64-target.h targmatch.h
CLEANFILES+=	bfd.h elf32-target.h elf64-target.h targmatch.h

.PATH:		${DIST}/bfd ${DIST}/opcodes ${DIST}/libiberty ${DIST}/include

# XXX temporary change mips -> mipsel
.include  "_${MACHINE_ARCH:S/^mips$/mipsel/}.mk"

bfd.h: bfd-in2.h bfd-h.sed
	sed -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.h} >$@

targmatch.h: config.bfd targmatch.sed
	sed -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.bfd} >$@

elf32-target.h: elfxx-target.h
	sed -e s/NN/32/g < $> > $@

elf64-target.h: elfxx-target.h
	sed -e s/NN/64/g < $> > $@

.include <bsd.lib.mk>

${OBJS}: bfd.h elf32-target.h elf64-target.h
targets.o targets.so targets.po: targmatch.h _${MACHINE_ARCH:S/^mips$/mipsel/}.mk
