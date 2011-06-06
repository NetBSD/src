# $NetBSD: Makefile.inc,v 1.8.6.1 2011/06/06 09:06:03 jruoho Exp $

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
