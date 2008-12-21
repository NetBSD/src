# $NetBSD: Makefile.inc,v 1.7 2008/12/21 11:48:30 ad Exp $

CFLAGS+=	-G 0 -mno-abicalls -ffixed-23

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-mno-abicalls -x assembler-with-cpp -traditional-cpp ${AOPTS}
