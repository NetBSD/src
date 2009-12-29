#	$NetBSD: Makefile,v 1.41 2009/12/29 20:21:45 elad Exp $

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
.SUFFIXES: .out .o .po .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

FILES= boot.amiga
BINDIR=/usr/mdec

COBJS = main.o console.o xd.o twiddle.o bzero.o gets.o
COBJS+=  lseek.o open.o read.o close.o dev.o errno.o
COBJS+=  ufs.o ustarfs.o panic.o vers.o files.o
COBJS+=  divdi3.o moddi3.o qdivrem.o

SOBJS = alloc.o ashrdi3.o ashldi3.o bcopy.o muldi3.o printf.o startit.o
SOBJS += strlen.o memcmp.o memset.o strerror.o strncmp.o
SOBJS += libstubs.o

OBJS=	$(SOBJS) $(COBJS)

#XX#DEFS = -D_STANDALONE -DSA_EXEC_ANYOWNER -DDYNAMIC_CRC_TABLE -DNOBYFOUR -UBYFOUR 
DEFS = -D_STANDALONE -DSA_EXEC_ANYOWNER 
DEFS += -D__INTERNAL_LIBSA_CREAD
#DEFS += -DSERCONSOLE
SOBJS += cread.o

#XX#SOBJS += adler32.o crc32.o inflate.o trees.o \
#XX#       zutil.o infback.o inftrees.o inffast.o

#DEFS += -DBOOT_ELF32 -DBOOT_AOUT
SOBJS += loadfile.o loadfile_aout.o loadfile_elf32.o

NETBSD_VERS!=	${HOST_SH} ${.CURDIR}/../../../../../conf/osrelease.sh
DEFS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

### main target: ###

realall: machine m68k ${FILES}

CLEANFILES += boot.amiga x.out xxstart.o libboot.a vers.c machine m68k

.include <bsd.prog.mk>

### special  rules for bootblocks ###

INCPATH = -nostdinc -I${DIR_TOP} -I${DIR_TOP}/lib/libsa
INCPATH += -I${DIR_TOP}/../common/dist/zlib
INCPATH += -I${.CURDIR}
INCPATH += -I${.CURDIR}/../../.. -I${.OBJDIR}
INCPATH += -I${.CURDIR}/../aout2bb

AFLAGS += -m68030 -l
CAFLAGS += -Wa,-l -Wa,-m68030 ${INCPATH}

COPTIM= -Os -fomit-frame-pointer -fcse-follow-jumps -fcse-skip-blocks  -Wa,-l -m68060 -Wa,-m68030
CFLAGS= -ffreestanding ${COPTIM} ${INCPATH} ${DEFS}
CFLAGS+= -Werror
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

.c.o:
	${CC} ${CFLAGS} -S $< -o $*.s
	${TXLT} < $*.s | ${AS} ${AFLAGS} -o $*.o
	rm $*.s

.s.o: ; ${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

.S.o: ; ${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

boot.amiga: x.out
	${RELOC2BB} -S x.out $@ || ${NM} -u x.out

x.out: xxstart.o libboot.a ${LIBZ}
	${LD} ${LDFLAGS} -r -dc -e _start -o $@ $>
	${SIZE} $@
	${NM} -u $@

xxstart.o: ${.CURDIR}/bbstart.s
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

libboot.a: ${OBJS}
	${AR} r $@ $> && ${RANLIB} $@

.PHONY:	vers.c
vers.c:	${.CURDIR}/version
	${HOST_SH} ${DIR_TOP}/conf/newvers_stand.sh -N ${.CURDIR}/version "amiga"

.if !make(obj) && !make(clean) && !make(cleandir)
.NOPATH: machine m68k
.BEGIN: machine m68k

machine:
	-rm -f $@
	ln -s ${DIR_TOP}/arch/amiga/include $@

m68k:
	-rm -f $@
	ln -s ${DIR_TOP}/arch/m68k/include $@
.endif

# make sure these are built:

${COBJS}: ${TXLT}
${FILES}: ${RELOC2BB}

.include "${.CURDIR}/../Makefile.booters"
