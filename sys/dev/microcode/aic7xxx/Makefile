#	$NetBSD: Makefile,v 1.2 1998/01/05 07:31:11 perry Exp $

aic7xxx_seq.h: aic7xxx_asm aic7xxx.seq
	./aic7xxx_asm -o ${.TARGET} aic7xxx.seq

aic7xxx_asm: aic7xxx_asm.o
	${CC} -o $@ aic7xxx_asm.o

aic7xxx_asm.o: aic7xxx_asm.c
