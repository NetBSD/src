#	$NetBSD: Makefile,v 1.1 1995/02/13 00:41:04 ragge Exp $
#

INCPATH=-I. -I../../.. -I../.. -I../../../lib/libsa

CC=	cc
AS=	as

RELOC=	100000

CFLAGS=	-O ${INCPATH} -DSTANDALONE
MACH=	-DVAX750

DRIVERS=autoconf.c hp.c hpmaptype.c ht.c idc.c kdb.c mba.c mt.c qdcons.c \
	qvcons.c rk.c rl.c tm.c tmscp.c ts.c uba.c uda.c up.c upmaptype.c \
	ut.c
DEVS=	autoconf.o hp.o
LIBS=	libsa.a libkern.a libsvax.a

all:	boot

libsa.a: ../../../lib/libsa/libsa.a
	cd ../../../lib/libsa; rm -f machine; \
	ln -s ../../arch/vax/include machine;make
	ln -s ../../../lib/libsa/libsa.a

libkern.a: ../../../lib/libkern/libkern.a
	cd ../../../lib/libkern; make
	ln -s ../../../lib/libkern/libkern.a

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

autoconf.o:	autoconf.c
	${CC} -c ${CFLAGS} $*.c

conf.o:	conf.c
	${CC} -c ${CFLAGS} $*.c

boot.o:	boot.c
	${CC} -c ${CFLAGS} $*.c

install:
	install -c -s -o bin -g bin -m 644 \
	    format cat copy drtest ls ${DESTDIR}/stand
	[ -d ../vaxdist/tp ] || mkdir ../vaxdist/tp
	cp tpcopy ../vaxdist/tp/copy
	cp tpboot ../vaxdist/tp/boot
	cp tpformat ../vaxdist/tp/format
	cp boot a.out; strip a.out; \
		dd if=a.out of=../floppy/boot bs=32 skip=1; rm a.out
	cp 730boot a.out; strip a.out; \
		dd if=a.out of=../cassette/boot.730 bs=32 skip=1; rm a.out
	cp ../floppy/boot ../cassette/boot.750
	cp ../floppy/boot ../consolerl/boot
	cp copy a.out; strip a.out; \
		dd if=a.out of=../floppy/copy bs=32 skip=1; rm a.out
	cp 730copy a.out; strip a.out; \
		dd if=a.out of=../cassette/copy.730 bs=32 skip=1; rm a.out
	cp ../floppy/copy ../cassette/copy.750
	cp ../floppy/copy ../consolerl/copy
	cp format a.out; strip a.out; \
		dd if=a.out of=../floppy/format bs=32 skip=1; rm a.out
	cp 730format a.out; strip a.out; \
		dd if=a.out of=../cassette/format.730 bs=32 skip=1; rm a.out
	cp ../floppy/format ../cassette/format.750
	cp ../floppy/format ../consolerl/format
	cp drtest a.out; strip a.out; \
		dd if=a.out of=../floppy/drtest bs=32 skip=1; rm a.out
	cp 730drtest a.out; strip a.out; \
		dd if=a.out of=../cassette/drtest.730 bs=32 skip=1; rm a.out
	cp ../floppy/drtest ../cassette/drtest.750
	cp ../floppy/drtest ../consolerl/drtest
	install -c -o bin -g bin -m 444 ${MDEC} ${DESTDIR}/usr/mdec
	rm -f ${DESTDIR}/usr/mdec/bootrd
	ln ${DESTDIR}/usr/mdec/bootra ${DESTDIR}/usr/mdec/bootrd

lint: ${SRCS}
	lint ${COPTS} -hxbn boot.c ${SRCS} | \
	    sed -e '/possible pointer alignment/d' \
		-e '/struct\/union .* never defined/d'

tags: ${SRCS}
	ctags ${SRCS} ${DUMMIES}
