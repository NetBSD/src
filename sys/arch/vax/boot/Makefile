#	$NetBSD: Makefile,v 1.2 1995/03/29 21:24:01 ragge Exp $
#

INCPATH=-I. -I../../.. -I../.. -I../../../lib/libsa

CC=	cc
AS=	as

RELOC=	100000

CFLAGS=	-O ${INCPATH} -DSTANDALONE -DRELOC=0x${RELOC}
MACH=	-DVAX750

DEVS=	autoconf.o hp.o ra.o
LIBS=	libsa.a libsvax.a libkern.a

all:	boot xxboot bootxx

libsa.a: ../../../lib/libsa/libsa.a
	@rm -f libsa.a
	cd ../../../lib/libsa; rm -f machine; \
	ln -s ../../arch/vax/include machine;make
	@ln -s ../../../lib/libsa/libsa.a .

libkern.a: ../../../lib/libkern/libkern.a
	@rm -f libkern.a
	cd ../../../lib/libkern; make
	@ln -s ../../../lib/libkern/libkern.a .

libsvax.a: consio.o urem.o udiv.o
	ar crv $@ $?
	ranlib $@

urem.o:	../vax/urem.s
	${CC} -x assembler-with-cpp -E ../vax/urem.s | as -o urem.o

udiv.o:	../vax/udiv.s
	${CC} -x assembler-with-cpp -E ../vax/udiv.s | as -o udiv.o

# startups

srt0.o:	srt0.s
	${CC} -x assembler-with-cpp -E -DREL srt0.s | as -o srt0.o


boot:	boot.o srt0.o devopen.o conf.o ${DEVS} ${LIBS} 
	ld -N -T ${RELOC} -e nisse -o $@ srt0.o devopen.o boot.o \
	conf.o ${DEVS} ${LIBS}
	@strip boot
	@size boot

hp.o:	hp.c
	${CC} -c ${CFLAGS} $*.c

ra.o:	ra.c
	${CC} -c ${CFLAGS} $*.c

autoconf.o:	autoconf.c
	${CC} -c ${CFLAGS} $*.c

conf.o:	conf.c
	${CC} -c ${CFLAGS} $*.c

boot.o:	boot.c
	${CC} -c ${CFLAGS} $*.c

bootblocks:	start.o bootxx.o init.o romread.o ${LIBS}
	ld -N -T ${RELOC} -o a.out start.o bootxx.o init.o romread.o ${LIBS}
	@strip a.out
	@size a.out
	@dd if=a.out of=bootblocks bs=32 skip=1
	@rm -f a.out

xxboot:	bootblocks
	dd if=bootblocks of=xxboot bs=512 count=1

bootxx:	bootblocks
	dd if=bootblocks of=bootxx bs=512 skip=1

start.o: start.s
	${CC} -x assembler-with-cpp -E start.s | as -o start.o

romread.o:	romread.s
	${CC} -x assembler-with-cpp -E romread.s | as -o romread.o

init.o:	init.c
	${CC} -c ${CFLAGS} $*.c

bootxx.o: bootxx.c 
	${CC} -c ${CFLAGS} $*.c

clean:
	rm -f start.o romread.o bootxx.o init.o xxboot bootxx bootblocks \
	libsvax.a libsa.a libkern.a udiv.o urem.o consio.o
	rm -f autoconf.o conf.o boot.o hp.o boot srt0.o devopen.o
