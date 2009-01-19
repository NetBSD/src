# $NetBSD: Makefile.inc,v 1.6.72.1 2009/01/19 13:16:30 skrll Exp $

CFLAGS+=	-G 0 -mno-abicalls -ffixed-23

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-mno-abicalls -x assembler-with-cpp -traditional-cpp ${AOPTS}
