#	$NetBSD: Makefile.inc,v 1.4.4.1 1996/08/25 17:22:28 thorpej Exp $

.if target(aic7xxx.o)
PATH: $S/dev/microcode/aic7xxx

aic7xxx.o: aic7xxx_seq.h

aic7xxx_seq.h: aic7xxx_asm $S/dev/microcode/aic7xxx/aic7xxx.seq
	./aic7xxx_asm -o ${.TARGET} $S/dev/microcode/aic7xxx/aic7xxx.seq

aic7xxx_asm: $S/dev/microcode/aic7xxx/aic7xxx_asm.c

.depend: aic7xxx_seq.h

clean::
	rm -f aic7xxx_asm aic7xxx_seq.h

.endif
