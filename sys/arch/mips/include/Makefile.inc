# $NetBSD: Makefile.inc,v 1.10 2016/06/09 05:43:51 martin Exp $

CFLAGS+=	-G 0 -ffixed-24

# this should really be !(RUMPKERNEL && target=PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mno-abicalls
AFLAGS+=	-mno-abicalls
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-x assembler-with-cpp ${AOPTS}
