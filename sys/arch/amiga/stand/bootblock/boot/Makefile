#	$NetBSD: Makefile,v 1.30 2003/02/03 07:16:08 mhitch Exp $

### what we need:

DIR_TOP=	${.CURDIR}/../../../../..
DIR_SA = 	${DIR_TOP}/lib/libsa
DIR_LIBZ =	${DIR_TOP}/lib/libz
DIR_KERN=	${DIR_TOP}/lib/libkern
DIR_KERN_MD=	${DIR_TOP}/lib/libkern/arch/$(MACHINE_ARCH)

.PATH:  $(DIR_SA) ${DIR_LIBZ} $(DIR_KERN) $(DIR_KERN_MD)

# prefer our assembler versions over assembler, and assembler over C:

.SUFFIXES:
.SUFFIXES: .out .o .po .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

FILES= boot.amiga
BINDIR=/usr/mdec

COBJS = main.o console.o xd.o twiddle.o bzero.o gets.o
COBJS+=  lseek.o open.o read.o close.o dev.o errno.o
COBJS+=  ufs.o ustarfs.o panic.o vers.o files.o
COBJS+=  divdi3.o moddi3.o qdivrem.o snprintf.o

SOBJS = alloc.o ashrdi3.o ashldi3.o bcopy.o muldi3.o printf.o startit.o
SOBJS += strlen.o strcmp.o memcmp.o strerror.o sprintf.o subr_prf.o
SOBJS += libstubs.o memset.o strncmp.o

OBJS=	$(SOBJS) $(COBJS)

DEFS = -D_STANDALONE -DINSECURE
DEFS += -D__INTERNAL_LIBSA_CREAD
SOBJS += cread.o
SOBJS += crc32.o inflate.o adler32.o infblock.o infcodes.o inftrees.o # XXX need libz!!!
SOBJS += inffast.o infutil.o
#DEFS += -DBOOT_ELF32 -DBOOT_AOUT
SOBJS += loadfile.o loadfile_aout.o loadfile_elf32.o

NETBSD_VERS!=	sh ${.CURDIR}/../../../../../conf/osrelease.sh
DEFS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

### main target: ###

realall: machine m68k ${FILES}

CLEANFILES += boot.amiga x.out xxstart.o libboot.a vers.c machine m68k

.include <bsd.prog.mk>

### special  rules for bootblocks ###

INCPATH = -nostdinc -I${DIR_TOP} -I${DIR_TOP}/lib/libsa -I${.CURDIR}
INCPATH += -I${.CURDIR}/../../.. -I${.OBJDIR}
INCPATH += -I${.CURDIR}/../aout2bb -I${DESTDIR}/usr/include

AFLAGS += -m68030 -l
CAFLAGS += -Wa,-l -Wa,-m68030 ${INCPATH}

COPTIM= -O -fomit-frame-pointer -fcse-follow-jumps -fcse-skip-blocks  -Wa,-l -m68060 -Wa,-m68030
CFLAGS= -ffreestanding ${COPTIM} ${INCPATH} ${DEFS} -Wall #-Wstrict-prototypes

.c.o:
	${CC} ${CFLAGS} -S $< -o $*.s
	${TXLT} < $*.s | ${AS} ${AFLAGS} -o $*.o
	rm $*.s

.s.o: ; ${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

.S.o: ; ${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

boot.amiga: x.out
	${RELOC2BB} -S x.out $@ || ${NM} -u x.out

x.out: xxstart.o libboot.a
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
	sh ${DIR_TOP}/conf/newvers_stand.sh -N ${.CURDIR}/version "amiga"

machine:
	-rm -f $@
	ln -s ${DIR_TOP}/arch/amiga/include $@

m68k:
	-rm -f $@
	ln -s ${DIR_TOP}/arch/m68k/include $@

# make sure these are built:

${COBJS}: ${TXLT}
${FILES}: ${RELOC2BB}

.include "${.CURDIR}/../Makefile.booters"
