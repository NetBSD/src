#	$NetBSD: Makefile,v 1.5 2002/05/31 20:57:42 thorpej Exp $

### what we need:

DIR_TOP=	${.CURDIR}/../../../../..
DIR_SA = 	${DIR_TOP}/lib/libsa
DIR_KERN=	${DIR_TOP}/lib/libkern
DIR_KERN_MD=	${DIR_TOP}/lib/libkern/arch/$(MACHINE_ARCH)
DIR_BOOT=	${.CURDIR}/../boot

.PATH:  $(DIR_SA) $(DIR_KERN) $(DIR_KERN_MD) $(DIR_BOOT)

# prefer our assembler versions over assembler, and assembler over C:

.SUFFIXES:
.SUFFIXES: .out .o .po .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

FILES= xxppcboot fdppcboot
BINDIR=/usr/mdec

COBJS = main.o console.o xd.o twiddle.o bzero.o gets.o
COBJS+=  lseek.o open.o read.o close.o dev.o errno.o
COBJS+=  ufs.o ppcstart.o

SOBJS = alloc.o ashrdi3.o bcopy.o muldi3.o printf.o
SOBJS += strlen.o strcmp.o
SOBJS += libstubs.o

OBJS=	$(SOBJS) $(COBJS)

DEFS = -DPPCBOOTER -D_STANDALONE -DINSECURE -DDEBUG_MEMORY_LIST

### main target: ###

all: ${FILES}

.include <bsd.prog.mk>

### special  rules for bootblocks ###

INCPATH = -I${DIR_TOP} -I${DIR_TOP}/lib/libsa -I${DIR_BOOT} -I${.CURDIR} -I${.CURDIR}/../../.. -I${.CURDIR}/../aout2bb -I${.OBJDIR}

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

CLEANFILES += xxppcboot fdppcboot x.out f.out xxstart.o fdstart.o libboot.a

xxppcboot: x.out
	${RELOC2BB} x.out $@ || nm -u x.out

fdppcboot: f.out
	${RELOC2BB} f.out $@ || nm -u f.out

x.out: xxstart.o libboot.a
	${LD} ${LDFLAGS} -r -dc -e _start -o $@ $>
	size $@
	nm -u $@

f.out: fdstart.o libboot.a
	${LD} ${LDFLAGS} -r -dc -e _start -o $@ $>
	size $@
	nm -u $@

xxstart.o: ${DIR_BOOT}/bbstart.s
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

fdstart.o: ${DIR_BOOT}/bbstart.s
	${CC} -DAUTOLOAD=8192 ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

PPCCC=powerpc-netbsd-gcc
PPCAS=powerpc-netbsd-as
PPCLD=powerpc-netbsd-ld
PPCOD=powerpc-netbsd-objdump

CLEANFILES += kickstart.o kickstart.bin kickstart68.c

ppcstart.o:	kickstart68.c

kickstart68.c:	kickstart.bin hex2c.awk
		hexdump kickstart.bin | awk -f ${.CURDIR}/hex2c.awk > $@

kickstart.bin:	kickstart.o
		$(PPCLD) -Ttext=0xfff00100 -o$@ --oformat binary $?

kickstart.o:	kickstart.S
		$(PPCCC) -DDEBUG $(COPTS) -x assembler-with-cpp -c $?

kickstart.lis:	kickstart.o
		$(PPCOD) --disassemble-all kickstart.o

libboot.a: ${OBJS}
	${AR} r $@ $> && ${RANLIB} $@

# make sure these are built:

${COBJS}: ${TXLT}
${FILES}: ${RELOC2BB}

.include "${.CURDIR}/../Makefile.booters"
