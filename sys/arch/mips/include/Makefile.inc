# $NetBSD: Makefile.inc,v 1.6.64.2 2010/03/11 15:02:38 yamt Exp $

CFLAGS+=	-G 0 -ffixed-23

# this should really be !(RUMPKERNEL && target=PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mno-abicalls
AFLAGS+=	-mno-abicalls
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-x assembler-with-cpp -traditional-cpp ${AOPTS}
