# $NetBSD: Makefile.inc,v 1.11 2021/03/23 11:56:55 simonb Exp $

CFLAGS+=	-G 0 -ffixed-24

# this should really be !(RUMPKERNEL && target=PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mno-abicalls
AFLAGS+=	-mno-abicalls
.endif

AFLAGS+=	-x assembler-with-cpp ${AOPTS}
