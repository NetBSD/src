# $NetBSD: Makefile.inc,v 1.10.30.1 2021/04/03 22:28:31 thorpej Exp $

CFLAGS+=	-G 0 -ffixed-24

# this should really be !(RUMPKERNEL && target=PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mno-abicalls
AFLAGS+=	-mno-abicalls
.endif

AFLAGS+=	-x assembler-with-cpp ${AOPTS}
