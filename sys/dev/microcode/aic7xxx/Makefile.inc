#	$NetBSD: Makefile.inc,v 1.7 1996/08/10 08:24:59 mycroft Exp $

.if target(aic7xxx.o)
PATH: $S/dev/microcode/aic7xxx

aic7xxx.o: aic7xxx_seq.h

aic7xxx_seq.h: aic7xxx_asm $S/dev/microcode/aic7xxx/aic7xxx.seq
	./aic7xxx_asm -o ${.TARGET} $S/dev/microcode/aic7xxx/aic7xxx.seq

aic7xxx_asm: aic7xxx_asm.o
	${CC} -o $@ aic7xxx_asm.o

aic7xxx_asm.o: $S/dev/microcode/aic7xxx/aic7xxx_asm.c
	${HOSTED_C_C}

.depend: aic7xxx_seq.h

clean::
	rm -f aic7xxx_asm aic7xxx_seq.h

.endif
