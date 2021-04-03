# $NetBSD: Makefile.inc,v 1.10.32.1 2021/04/03 21:44:45 thorpej Exp $

CFLAGS+=	-G 0 -ffixed-24

# this should really be !(RUMPKERNEL && target=PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mno-abicalls
AFLAGS+=	-mno-abicalls
.endif

AFLAGS+=	-x assembler-with-cpp ${AOPTS}
