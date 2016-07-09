# $NetBSD: Makefile.inc,v 1.9.32.1 2016/07/09 20:24:53 skrll Exp $

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
