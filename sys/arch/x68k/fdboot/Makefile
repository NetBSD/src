#	$NetBSD: Makefile,v 1.1.1.1 1996/05/05 12:17:03 oki Exp $

#G=-g
#M=-m68020 -fall-bsr
CFLAGS=-O2 -fomit-frame-pointer -Wall $M $G

all: fdboot

fdboot: fdboot.o bootufs.o
	@echo loading fdboot
	@ld -n -Bstatic -Ttext 0x3f0000 -o fdboot.x fdboot.o bootufs.o -lc
	@cp fdboot.x s.x
	@strip s.x
	@dd bs=1 skip=32 count=8192 if=s.x of=fdboot 2> /dev/null
	@rm s.x
	@size fdboot.x

bootufs.o: bootufs.c fdboot.h
	$(CC) -o $@ -c -I . $< $(CFLAGS)

fdboot.o : fdboot.s
	$(CC) -o $@ -c $<

.ifndef ID
installboot: fdboot
	@echo setenv ID before doing that.
	@false
.else
installboot: fdboot
	./writefdboot fdboot /dev/rfd${ID}a
.endif

install: fdboot
	install -c -o bin -g bin -m 444 fdboot ${DESTDIR}/usr/mdec

clean::
	rm -f fdboot fdboot.o bootufs.o fdboot.x

.include <bsd.prog.mk>
