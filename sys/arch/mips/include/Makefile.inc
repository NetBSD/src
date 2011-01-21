# $NetBSD: Makefile.inc,v 1.9 2011/01/21 15:59:07 joerg Exp $

CFLAGS+=	-G 0 -ffixed-23

# this should really be !(RUMPKERNEL && target=PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mno-abicalls
AFLAGS+=	-mno-abicalls
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-x assembler-with-cpp ${AOPTS}
