#	$NetBSD: Makefile,v 1.3 1996/10/18 06:10:58 thorpej Exp $

#G=-g
#M=-m68020 -fall-bsr
CFLAGS=-O2 -fomit-frame-pointer -Wall $M $G

all: fdboot

fdboot: fdboot.o bootufs.o
	@echo loading fdboot
	@$(LD) -n -Bstatic -Ttext 0x3f0000 -o fdboot.x fdboot.o bootufs.o -lc
	@cp fdboot.x s.x
	@strip s.x
	@dd bs=1 skip=32 count=8192 if=s.x of=fdboot 2> /dev/null
	@rm s.x
	@size fdboot.x

bootufs.o: bootufs.c fdboot.h
	$(CC) -o $@ -c $< $(CFLAGS)

fdboot.o : fdboot.s chkfmt.s
	$(CPP) ${.CURDIR}/fdboot.s | $(AS) -o $@

.ifndef ID
installboot: fdboot
	@echo setenv ID before doing that.
	@false
.else
installboot: fdboot
	${.CURDIR}/writefdboot fdboot /dev/rfd${ID}a
.endif

install: fdboot
	${INSTALL} -c -o bin -g bin -m 444 fdboot ${DESTDIR}/usr/mdec

clean::
	rm -f fdboot fdboot.o bootufs.o fdboot.x

.include <bsd.prog.mk>
