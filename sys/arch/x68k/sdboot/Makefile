#	$NetBSD: Makefile,v 1.2 1996/10/18 06:11:29 thorpej Exp $

#G=-g
#M=-m68020 -fall-bsr
CFLAGS=-O2 -fomit-frame-pointer -Wall $M $G

all: sdboot

sdboot: sdboot.o bootufs.o
	@echo loading sdboot
	@ld -n -Bstatic -Ttext 0x3f0000 -o sdboot.x sdboot.o bootufs.o -lc
	@cp sdboot.x s.x
	@strip s.x
	@dd bs=1 skip=32 count=8192 if=s.x of=sdboot 2> /dev/null
	@rm s.x
	@size sdboot.x

bootufs.o: bootufs.c sdboot.h
	$(CC) -o $@ -c -I . $< $(CFLAGS)

sdboot.o : sdboot.s
	$(CC) -o $@ -c $<

.ifndef ID
installboot: sdboot
	@echo setenv ID before doing that.
	@false
.else
installboot: sdboot
	cat sdboot > /dev/rsd${ID}a
.endif

install: sdboot
	${INSTALL} -c -o bin -g bin -m 444 sdboot ${DESTDIR}/usr/mdec

clean::
	rm -f sdboot sdboot.o bootufs.o sdboot.x

.include <bsd.prog.mk>
