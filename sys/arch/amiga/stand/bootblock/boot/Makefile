#	$NetBSD: Makefile,v 1.20.4.2 2002/02/09 17:35:23 he Exp $

### what we need:

DIR_TOP=	${.CURDIR}/../../../../..
DIR_SA = 	${DIR_TOP}/lib/libsa
DIR_KERN=	${DIR_TOP}/lib/libkern
DIR_KERN_MD=	${DIR_TOP}/lib/libkern/arch/$(MACHINE_ARCH)

.PATH:  $(DIR_SA) $(DIR_KERN) $(DIR_KERN_MD)

# prefer our assembler versions over assembler, and assembler over C:

.SUFFIXES:
.SUFFIXES: .out .o .po .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

FILES= xxboot fdboot
BINDIR=/usr/mdec

COBJS = main.o console.o xd.o twiddle.o bzero.o gets.o
COBJS+=  lseek.o open.o read.o close.o dev.o errno.o
COBJS+=  ufs.o

SOBJS = alloc.o ashrdi3.o ashldi3.o bcopy.o muldi3.o printf.o startit.o
SOBJS += strlen.o strcmp.o
SOBJS += libstubs.o 

OBJS=	$(SOBJS) $(COBJS)

DEFS = -D_STANDALONE -DINSECURE

### main target: ###

realall: ${FILES}

.include <bsd.prog.mk>

### special  rules for bootblocks ###

INCPATH = -nostdinc -I${DIR_TOP} -I${DIR_TOP}/lib/libsa -I${.CURDIR} -I${.CURDIR}/../../.. -I${.CURDIR}/../aout2bb -I${DESTDIR}/usr/include

AFLAGS += -m68030 -l
CAFLAGS += -Wa,-l -Wa,-m68030 ${INCPATH}

COPTIM= -O -fomit-frame-pointer -fcse-follow-jumps -fcse-skip-blocks  -Wa,-l -m68060 -Wa,-m68030
CFLAGS= ${COPTIM} ${INCPATH} ${DEFS} -Wall #-Wstrict-prototypes

.c.o:
	${CC} ${CFLAGS} -S $< -o $*.s
	${TXLT} < $*.s | ${AS} ${AFLAGS} -o $*.o
	rm $*.s

.s.o: ; ${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

.S.o: ; ${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

CLEANFILES += xxboot fdboot x.out f.out xxstart.o fdstart.o libboot.a

xxboot: x.out
	${AOUT2BB} x.out $@ || nm -u x.out

fdboot: f.out
	${AOUT2BB} f.out $@ || nm -u f.out

x.out: xxstart.o libboot.a
	${LD} ${LDFLAGS} -r -dc -e _start -o $@ $>
	size $@
	nm -u $@

f.out: fdstart.o libboot.a
	${LD} ${LDFLAGS} -r -dc -e _start -o $@ $>
	size $@
	nm -u $@

xxstart.o: ${.CURDIR}/bbstart.s
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

fdstart.o: ${.CURDIR}/bbstart.s
	${CC} -DAUTOLOAD=8192 ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

libboot.a: ${OBJS}
	${AR} r $@ $> && ${RANLIB} $@

# make sure these are built:

${COBJS}: ${TXLT}
${FILES}: ${AOUT2BB}
