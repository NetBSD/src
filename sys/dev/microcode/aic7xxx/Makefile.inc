#	$NetBSD: Makefile.inc,v 1.6 1996/07/13 06:20:11 explorer Exp $

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
