MAN=installboot.8

### what we need:

S=		${.CURDIR}/../../../..
DIR_SA = 	${S}/lib/libsa
DIR_KERN=	${S}/lib/libkern
DIR_KERN_MD=	${S}/lib/libkern/arch/$(MACHINE_ARCH)

.PATH:  $(DIR_SA) $(DIR_KERN) $(DIR_KERN_MD)

# prefer our assembler versions over assembler, and assembler over C:

.SUFFIXES:
.SUFFIXES: .out .o .po .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

SRCS=installboot.sh

BOOTBLOCKS= xxboot fdboot

COBJS = configure.o main.o console.o xd.o twiddle.o bzero.o gets.o
COBJS+=  lseek.o open.o read.o close.o dev.o
COBJS+=  ufs.o

SOBJS = alloc.o ashrdi3.o bcopy.o muldi3.o printf.o startit.o
SOBJS += strlen.o strcmp.o
SOBJS += libstubs.o 

OBJS=	$(SOBJS) $(COBJS)

DEFS = -DSTANDALONE -DINSECURE

### main target: ###

all: ${BOOTBLOCKS} installboot

### special  rules for bootblocks ###

AFLAGS += -m68030 -l
CAFLAGS += -Wa,-l -Wa,-m68030

INCPATH = -I${S} -I${S}/lib/libsa -I${.CURDIR} -I${.CURDIR}/../..
BB_COPTIM= -O2 -fomit-frame-pointer -fno-function-cse -Wa,-l -m68060 -Wa,-m68030
BB_CFLAGS = ${BB_COPTIM} ${INCPATH} ${DEFS} -Wall #-Wstrict-prototypes

.c.o:
	$(CC) $(BB_CFLAGS) -S $< -o $*.s
	./txlt < $*.s | $(AS) $(AFLAGS) -o $*.o
	rm $*.s

.s.o: ; $(CC) $(CAFLAGS) $(COPTS) -x assembler-with-cpp -o $@ -c $<

.S.o: ; $(CC) $(CAFLAGS) $(COPTS) -x assembler-with-cpp -o $@ -c $<

${COBJS}: txlt

${BOOTBLOCKS}: aout2bb

CLEANFILES += xxboot fdboot x.out f.out xxstart.o fdstart.o libboot.a

xxboot: x.out
	./aout2bb x.out $@ || nm -u x.out

fdboot: f.out
	./aout2bb f.out $@ || nm -u f.out

x.out: xxstart.o libboot.a
	$(LD) $(LDFLAGS) -r -dc -e _start -o $@ $>
	size $@
	nm -u $@

f.out: fdstart.o libboot.a
	$(LD) $(LDFLAGS) -r -dc -e _start -o $@ $>
	size $@
	nm -u $@

xxstart.o: ${.CURDIR}/bbstart.s
	$(CC) $(CAFLAGS) $(COPTS) -x assembler-with-cpp \
		-o $@ -c $>

fdstart.o: ${.CURDIR}/bbstart.s
	$(CC) -DAUTOLOAD=8192 $(CAFLAGS) $(COPTS) -x assembler-with-cpp \
		-o $@ -c $>

libboot.a: $(OBJS)
	ar r $@ $> && ranlib $@

### install what we need: ###

install:
	install -o $(BINOWN) -g $(BINGRP) installboot ${DESTDIR}/usr/sbin
	install -o $(BINOWN) -g $(BINGRP) $(BOOTBLOCKS) ${DESTDIR}/usr/mdec

### start of helper programs: ###

HOSTED_CC=	$(CC)
HOSTED_CFLAGS=	$(CFLAGS)

HOSTED_C=	${HOSTED_CC} ${HOSTED_CFLAGS} ${HOSTED_CPPFLAGS} -c $<

CLEANFILES += txlt txlt.o aout2bb aout2bb.o chksum.o

aout2bb: aout2bb.o chksum.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $>

aout2bb.o: ${.CURDIR}/aout2bb.c
	${HOSTED_C}

chksum.o: ${.CURDIR}/chksum.c
	${HOSTED_C}

txlt: txlt.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $> -ll

depend:
	mkdep -a ${.CURDIR}/aout2bb.c ${.CURDIR}/chksum.c

### end of helper programs ###

.include <bsd.prog.mk>
