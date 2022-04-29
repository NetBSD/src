#	$NetBSD: Makefile,v 1.60 2022/04/29 06:48:22 rin Exp $

.include <bsd.init.mk>
.include <bsd.sys.mk>		# for HOST_SH

### what we need:

#XX# lines replace similar lines, or are added, in case we want to
#XX# compile LIBZ ourselves to save a few bytes (think TXLT)

#XX#DIR_LIBZ =	${DIR_TOP}/../common/dist/zlib

DIR_TOP=	${.CURDIR}/../../../../..
DIR_SA = 	${DIR_TOP}/lib/libsa
DIR_LIBZ=
DIR_KERN=	${DIR_TOP}/lib/libkern
DIR_KERN_MD=	${DIR_TOP}/lib/libkern/arch/$(MACHINE_ARCH)
DIR_LIBC=	${DIR_TOP}/../common/lib/libc

.PATH:  $(DIR_SA) ${DIR_LIBZ} $(DIR_KERN) $(DIR_KERN_MD) \
	${DIR_LIBC}/gen ${DIR_LIBC}/arch/m68k/gen \
	${DIR_LIBC}/inet ${DIR_LIBC}/arch/m68k/inet \
	${DIR_LIBC}/quad ${DIR_LIBC}/arch/m68k/quad \
	${DIR_LIBC}/string ${DIR_LIBC}/arch/m68k/string

### find out what to use for libz

S=	${DIR_TOP}	# ????
Z_AS= library
.include "${DIR_TOP}/lib/libz/Makefile.inc"
LIBZ= ${ZLIB}

# prefer our assembler versions over assembler, and assembler over C:

.SUFFIXES:
.SUFFIXES: .out .o .po .pico .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

FILES= boot.amiga
BINDIR=/usr/mdec

COBJS = main.o console.o xd.o twiddle.o bzero.o gets.o
COBJS+=  lseek.o open.o read.o close.o dev.o errno.o
COBJS+=  ufs.o ustarfs.o ffsv2.o panic.o vers.o files.o
COBJS+=  divdi3.o moddi3.o qdivrem.o

SOBJS = alloc.o ashrdi3.o ashldi3.o bcopy.o muldi3.o printf.o startit.o
SOBJS += strlen.o memcmp.o memset.o strerror.o strncmp.o
SOBJS += libstubs.o

OBJS+=	$(SOBJS) $(COBJS)

#XX#DEFS = -D_STANDALONE -DSA_EXEC_ANYOWNER -DDYNAMIC_CRC_TABLE -DNOBYFOUR -UBYFOUR 
DEFS = -D_STANDALONE -DSA_EXEC_ANYOWNER 
DEFS += -D__INTERNAL_LIBSA_CREAD
DEFS += -DSERCONSOLE
SOBJS += cread.o

#XX#SOBJS += adler32.o crc32.o inflate.o trees.o \
#XX#       zutil.o infback.o inftrees.o inffast.o

#DEFS += -DBOOT_ELF32 -DBOOT_AOUT
SOBJS += loadfile.o loadfile_aout.o loadfile_elf32.o

NETBSD_VERS!=	${HOST_SH} ${.CURDIR}/../../../../../conf/osrelease.sh
DEFS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

### main target: ###

realall: ${FILES}

CLEANFILES += boot.amiga x.out xxstart.o libboot.a

VERSIONFLAGS+=-n
.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
.include <bsd.klinks.mk>

### special  rules for bootblocks ###

INCPATH = -nostdinc -I${DIR_TOP} -I${DIR_TOP}/lib/libsa
INCPATH += -I${DIR_TOP}/../common/dist/zlib
INCPATH += -I${.CURDIR}
INCPATH += -I${.CURDIR}/../../.. -I${.OBJDIR}
INCPATH += -I${.CURDIR}/../elf2bb

AFLAGS += -march=68030 -mcpu=68030
CAFLAGS += -Wa,-march=68030 -Wa,-mcpu=68030 ${INCPATH}

COPTIM= -Os -fomit-frame-pointer -fcse-follow-jumps -fcse-skip-blocks
COPTIM+= -m68060 -Wa,-march=68030 -Wa,-mcpu=68030 -fno-unwind-tables
CFLAGS= -ffreestanding ${COPTIM} ${INCPATH} ${DEFS}
CFLAGS+= -Werror
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

# XXX work around a gcc 4.5 bug
COPTS.cread.c = -O1

.c.o:
	${_MKTARGET_COMPILE}
	${CC} ${CFLAGS} ${COPTS.${.IMPSRC:T}} -S $< -o $*.s
	${TXLT} < $*.s | ${AS} ${AFLAGS} -o $@
	rm $*.s

.s.o:
	${_MKTARGET_COMPILE}
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

.S.o:
	${_MKTARGET_COMPILE}
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

boot.amiga: x.out
	${_MKTARGET_CREATE}
	${RELOC2BB} -S x.out $@ || (${NM} -u x.out && false)

x.out: xxstart.o libboot.a ${LIBZ}
	${_MKTARGET_LINK}
	${LD} ${LDFLAGS} -r -dc -e start -o $@ $>
	${SIZE} $@
	${NM} -u $@

xxstart.o: ${.CURDIR}/bbstart.s
	${_MKTARGET_COMPILE}
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

libboot.a: ${OBJS}
	${_MKTARGET_BUILD}
	${AR} crs $@ $> && ${RANLIB} $@

# make sure these are built:

${COBJS}: ${TXLT}
${FILES}: ${RELOC2BB}

.include "${.CURDIR}/../Makefile.booters"
